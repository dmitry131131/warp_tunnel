#include "parser.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>

namespace sim {
namespace {

enum class OperandKind { Register, Value, Label, AllocationSize };
enum class Suffix { None, Width, Comparison };

struct InstructionSpec {
    std::string_view name;
    Opcode opcode;
    Suffix suffix;
    std::vector<OperandKind> operands;
};

using Kind = OperandKind;

// One definition of each instruction's spelling, suffix and operand roles.
const std::array<InstructionSpec, 30> instruction_specs{{
    {"mov", Opcode::Mov, Suffix::None, {Kind::Register, Kind::Value}},
    {"add", Opcode::Add, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"sub", Opcode::Sub, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"mul", Opcode::Mul, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"sdiv", Opcode::Sdiv, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"udiv", Opcode::Udiv, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"srem", Opcode::Srem, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"shl", Opcode::Shl, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"ashr", Opcode::Ashr, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"or", Opcode::Or, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"sext32", Opcode::Sext32, Suffix::None, {Kind::Register, Kind::Value}},
    {"abs", Opcode::Abs, Suffix::None, {Kind::Register, Kind::Value}},
    {"umax", Opcode::Umax, Suffix::None, {Kind::Register, Kind::Value, Kind::Value}},
    {"clamp8", Opcode::Clamp8, Suffix::None, {Kind::Register, Kind::Value}},
    {"mdiv", Opcode::Mdiv, Suffix::Width, {Kind::Register, Kind::Value, Kind::Value}},
    {"alloc", Opcode::Alloc, Suffix::None, {Kind::Register, Kind::AllocationSize}},
    {"ld", Opcode::Ld, Suffix::None, {Kind::Register, Kind::Value, Kind::Value}},
    {"st", Opcode::St, Suffix::None, {Kind::Value, Kind::Value, Kind::Value}},
    {"jmp", Opcode::Jmp, Suffix::None, {Kind::Label}},
    {"bcmp", Opcode::Bcmp, Suffix::Comparison, {Kind::Value, Kind::Value, Kind::Label}},
    {"brect", Opcode::Brect, Suffix::None, {Kind::Value, Kind::Value, Kind::Label}},
    {"call", Opcode::Call, Suffix::None, {Kind::Label}},
    {"ret", Opcode::Ret, Suffix::None, {}},
    {"init", Opcode::Init, Suffix::None, {}},
    {"exit", Opcode::Exit, Suffix::None, {}},
    {"flush", Opcode::Flush, Suffix::None, {}},
    {"putpixel", Opcode::Putpixel, Suffix::None, {Kind::Value, Kind::Value, Kind::Value}},
    {"rand", Opcode::Rand, Suffix::None, {Kind::Register}},
    {"hasclick", Opcode::Hasclick, Suffix::None, {Kind::Register}},
    {"getclick", Opcode::Getclick, Suffix::None, {Kind::Register}},
}};

std::string_view trim(std::string_view text) {
    constexpr std::string_view whitespace = " \t\r\n\v\f";
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

bool is_label(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    const auto is_start = [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    };
    if (!is_start(name.front())) {
        return false;
    }
    return std::all_of(name.begin(), name.end(), [&](unsigned char c) {
        return is_start(c) || (c >= '0' && c <= '9');
    });
}

std::vector<std::string_view> split(std::string_view text, char separator) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    for (;;) {
        const auto end = text.find(separator, start);
        parts.push_back(trim(text.substr(start, end - start)));
        if (end == std::string_view::npos) {
            return parts;
        }
        start = end + 1;
    }
}

std::uint64_t parse_unsigned(std::string_view text, int base, std::size_t line) {
    if (text.empty()) {
        throw ParseError(line, "expected an integer literal");
    }
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw ParseError(line, "invalid or out-of-range integer literal");
    }
    return result;
}

Immediate parse_immediate(std::string_view token, std::size_t line) {
    const bool negative = token.front() == '-';
    if (negative) {
        token.remove_prefix(1);
    }
    int base = 10;
    if (token.substr(0, 2) == "0x" || token.substr(0, 2) == "0X") {
        base = 16;
        token.remove_prefix(2);
    }
    const auto magnitude = parse_unsigned(token, base, line);
    if (negative && magnitude > (std::uint64_t{1} << 63)) {
        throw ParseError(line, "negative literal is smaller than -2^63");
    }
    return Immediate{negative ? std::uint64_t{0} - magnitude : magnitude};
}

Register parse_register(std::string_view token, std::size_t line) {
    if (token.size() < 2 || token.front() != 'x') {
        throw ParseError(line, "expected a destination register x0..x63");
    }
    const auto index = parse_unsigned(token.substr(1), 10, line);
    if (index >= register_count) {
        throw ParseError(line, "register index must be in x0..x63");
    }
    return Register{static_cast<unsigned>(index)};
}

Operand parse_operand(std::string_view token, Kind kind, std::size_t line) {
    if (token.empty()) {
        throw ParseError(line, "empty operand");
    }
    if (kind == Kind::Label) {
        if (!is_label(token)) {
            throw ParseError(line, "invalid label name");
        }
        return LabelTarget{std::string(token), 0};
    }
    if (kind == Kind::Register) {
        return parse_register(token, line);
    }
    if (kind == Kind::Value && token.front() == 'x') {
        return parse_register(token, line);
    }

    const auto immediate = parse_immediate(token, line);
    if (kind == Kind::AllocationSize) {
        // The allocation must have a positive count and a representable byte size.
        if (token.front() == '-' || immediate.bits == 0 ||
            immediate.bits > std::numeric_limits<std::size_t>::max() / sizeof(std::uint64_t)) {
            throw ParseError(line, "alloc requires a positive literal with a representable byte size");
        }
    }
    return immediate;
}

Instruction parse_instruction(std::string_view text, std::size_t line) {
    const auto separator = text.find_first_of(" \t\r\v\f");
    const auto mnemonic = text.substr(0, separator);
    const auto suffixes = split(mnemonic, '.');
    const auto spec = std::find_if(instruction_specs.begin(), instruction_specs.end(),
                                   [&](const InstructionSpec& candidate) {
                                       return candidate.name == suffixes.front();
                                   });
    if (spec == instruction_specs.end()) {
        throw ParseError(line, "unknown instruction '" + std::string(mnemonic) + "'");
    }

    Instruction instruction{spec->opcode, 0, Predicate::None, {}, line};
    const std::size_t expected_parts = spec->suffix == Suffix::Comparison ? 3 :
                                       spec->suffix == Suffix::Width ? 2 : 1;
    if (suffixes.size() != expected_parts) {
        throw ParseError(line, "invalid suffix for '" + std::string(spec->name) + "'");
    }
    if (spec->suffix != Suffix::None) {
        const auto width = suffixes.back();
        if (width != "8" && width != "32" && width != "64") {
            throw ParseError(line, "instruction width must be 8, 32 or 64");
        }
        instruction.width = static_cast<unsigned>(parse_unsigned(width, 10, line));
    }
    if (spec->suffix == Suffix::Comparison) {
        const auto predicate = suffixes[1];
        if (predicate == "eq") {
            instruction.predicate = Predicate::Eq;
        } else if (predicate == "slt") {
            instruction.predicate = Predicate::Slt;
        } else if (predicate == "sgt") {
            instruction.predicate = Predicate::Sgt;
        } else {
            throw ParseError(line, "comparison predicate must be eq, slt or sgt");
        }
    }

    const auto operands_text = separator == std::string_view::npos ? std::string_view{} :
                               trim(text.substr(separator));
    const auto tokens = operands_text.empty() ? std::vector<std::string_view>{} :
                                               split(operands_text, ',');
    if (tokens.size() != spec->operands.size()) {
        throw ParseError(line, "'" + std::string(mnemonic) + "' expects " +
                         std::to_string(spec->operands.size()) + " operands, got " +
                         std::to_string(tokens.size()));
    }
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        instruction.operands.push_back(parse_operand(tokens[index], spec->operands[index], line));
    }
    return instruction;
}

void resolve_labels(Program& program) {
    for (const auto& [name, label] : program.labels) {
        if (label.instruction_index == program.instructions.size()) {
            throw ParseError(label.source_line, "label '" + name + "' has no instruction");
        }
    }
    for (auto& instruction : program.instructions) {
        for (auto& operand : instruction.operands) {
            auto* target = std::get_if<LabelTarget>(&operand);
            if (target == nullptr) {
                continue;
            }
            const auto label = program.labels.find(target->name);
            if (label == program.labels.end()) {
                throw ParseError(instruction.source_line, "undefined label '" + target->name + "'");
            }
            target->instruction_index = label->second.instruction_index;
        }
    }
    const auto entry = program.labels.find("app");
    if (entry != program.labels.end()) {
        program.entry_point = entry->second.instruction_index;
    }
}

} // namespace

ParseError::ParseError(std::size_t line, const std::string& message)
    : std::runtime_error(message), line_(line) {}

Program parse(std::string_view source) {
    Program program;
    std::size_t line_number = 0;
    std::size_t start = 0;
    while (start < source.size()) {
        ++line_number;
        const auto end = source.find('\n', start);
        const auto raw_line = source.substr(start, end - start);
        const auto line = trim(raw_line.substr(0, raw_line.find(';')));
        if (!line.empty()) {
            if (line.back() == ':') {
                const auto name = trim(line.substr(0, line.size() - 1));
                if (!is_label(name)) {
                    throw ParseError(line_number, "invalid label definition");
                }
                const auto [existing, inserted] = program.labels.emplace(
                    std::string(name), Label{program.instructions.size(), line_number});
                if (!inserted) {
                    throw ParseError(line_number, "duplicate label '" + std::string(name) +
                                     "' (first defined on line " +
                                     std::to_string(existing->second.source_line) + ")");
                }
            } else {
                program.instructions.push_back(parse_instruction(line, line_number));
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    if (program.instructions.empty()) {
        throw ParseError(1, "program contains no instructions");
    }
    resolve_labels(program);
    return program;
}

Program parse_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open input file");
    }
    const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
        throw std::runtime_error("cannot read input file");
    }
    return parse(source);
}

} // namespace sim
