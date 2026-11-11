#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace warp {

inline constexpr unsigned register_count = 64;

enum class Opcode {
    Mov, Add, Sub, Mul, Sdiv, Udiv, Srem, Shl, Ashr, Or,
    Sext32, Abs, Umax, Clamp8, Mdiv,
    Alloc, Ld, St, Jmp, Bcmp, Brect, Call, Ret,
    Init, Exit, Flush, Putpixel, Rand, Hasclick, Getclick
};

enum class Predicate { None, Eq, Slt, Sgt };

struct Register {
    unsigned index;
};

struct Immediate {
    // Negative literals are stored in two's complement.
    std::uint64_t bits;
};

struct LabelTarget {
    std::string name;
    std::size_t instruction_index;
};

using Operand = std::variant<Register, Immediate, LabelTarget>;

struct Instruction {
    Opcode opcode;
    unsigned width; // 8, 32 or 64 for arithmetic/comparisons, 0 otherwise.
    Predicate predicate;
    std::vector<Operand> operands;
    std::size_t source_line;
};

struct Label {
    std::size_t instruction_index;
    std::size_t source_line;
};

struct Program {
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, Label> labels;
    std::size_t entry_point = 0;
};

class ParseError : public std::runtime_error {
public:
    ParseError(std::size_t line, const std::string& message);
    std::size_t line() const noexcept { return line_; }

private:
    std::size_t line_;
};

// Parses the entire source and resolves label references before returning.
// Throws ParseError on invalid source. No partial Program is returned.
Program parse(std::string_view source);

// Reads the entire file before parsing. I/O errors throw std::runtime_error.
Program parse_file(const std::filesystem::path& path);

} // namespace warp
