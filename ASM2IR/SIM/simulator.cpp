#include "simulator.h"

#include <algorithm>
#include <iterator>
#include <limits>

extern "C" {
#include "sim.h"
}

namespace warp {
namespace {

std::uint64_t mask(unsigned width) {
    return std::numeric_limits<std::uint64_t>::max() >> (64 - width);
}

// Decode two's complement without out-of-range unsigned-to-signed casts.
std::int64_t signed_value(std::uint64_t value, unsigned width = 64) {
    value &= mask(width);
    if ((value & (std::uint64_t{1} << (width - 1))) != 0) {
        return -1 - static_cast<std::int64_t>((~value) & mask(width));
    }
    return static_cast<std::int64_t>(value);
}

} // namespace

ExecutionError::ExecutionError(std::size_t line, const std::string& message)
    : std::runtime_error(message), line_(line) {}

Simulator::Simulator(const Program& program, std::size_t memory_words)
    : program_(program), memory_(memory_words), initialized_(memory_words, false) {}

[[noreturn]] void Simulator::fail(const std::string& message) const {
    throw ExecutionError(source_line_, message);
}

std::uint64_t Simulator::read(const Operand& operand) const {
    if (const auto* reg = std::get_if<Register>(&operand)) {
        return registers_.at(reg->index);
    }
    return std::get<Immediate>(operand).bits;
}

void Simulator::write(const Instruction& instruction, std::uint64_t value) {
    registers_.at(std::get<Register>(instruction.operands[0]).index) = value;
}

std::size_t Simulator::target(const Operand& operand) const {
    const auto address = std::get<LabelTarget>(operand).instruction_index;
    if (address >= program_.instructions.size()) {
        fail("jump target is outside the program");
    }
    return address;
}

std::size_t Simulator::memory_index(std::uint64_t base, std::uint64_t index) const {
    if (base % sizeof(std::uint64_t) != 0 || base / 8 >= memory_.size()) {
        fail("invalid or unaligned memory base");
    }
    const auto word = static_cast<std::size_t>(base / 8);
    const auto after = std::upper_bound(
        allocations_.begin(), allocations_.end(), word,
        [](std::size_t address, const Allocation& allocation) { return address < allocation.begin; });
    if (after == allocations_.begin()) {
        fail("memory base does not belong to an allocation");
    }
    const auto& allocation = *std::prev(after);
    if (word >= allocation.end) {
        fail("memory base does not belong to an allocation");
    }

    // Check displacement before addition, so even extreme indices cannot wrap.
    if (signed_value(index) < 0) {
        const auto distance = std::uint64_t{0} - index;
        if (distance > word - allocation.begin) {
            fail("memory access is before the allocation");
        }
        return word - static_cast<std::size_t>(distance);
    }
    if (index >= allocation.end - word) {
        fail("memory access is past the allocation");
    }
    return word + static_cast<std::size_t>(index);
}

std::uint64_t Simulator::divide(std::uint64_t a, std::uint64_t b, unsigned width,
                                bool remainder) const {
    const auto dividend = signed_value(a, width);
    const auto divisor = signed_value(b, width);
    if (divisor == 0) {
        fail("division by zero");
    }
    const auto minimum = signed_value(std::uint64_t{1} << (width - 1), width);
    if (dividend == minimum && divisor == -1) {
        fail("signed division overflow");
    }
    const auto result = remainder ? dividend % divisor : dividend / divisor;
    return static_cast<std::uint64_t>(result) & mask(width);
}

void Simulator::ensure_sim() {
    if (!sim_initialized_) {
        simInit();
        sim_initialized_ = true;
    }
}

void Simulator::close_sim() {
    if (sim_initialized_) {
        simExit();
        sim_initialized_ = false;
    }
}

void Simulator::run() {
    registers_.fill(0);
    std::fill(initialized_.begin(), initialized_.end(), false);
    allocations_.clear();
    return_stack_.clear();
    next_word_ = 2;
    pc_ = program_.entry_point;
    source_line_ = 0;
    halted_ = false;

    try {
        while (!halted_) {
            if (pc_ >= program_.instructions.size()) {
                fail("execution reached the end of the program without ret");
            }
            const auto& instruction = program_.instructions[pc_];
            source_line_ = instruction.source_line;
            // Fall-through and return addresses both refer to the next instruction.
            ++pc_;
            dispatch(instruction);
        }
    } catch (...) {
        close_sim();
        throw;
    }
    close_sim();
}

void Simulator::dispatch(const Instruction& instruction) {
    switch (instruction.opcode) {
    case Opcode::Mov: return handle_mov(instruction);
    case Opcode::Add: return handle_add(instruction);
    case Opcode::Sub: return handle_sub(instruction);
    case Opcode::Mul: return handle_mul(instruction);
    case Opcode::Sdiv: return handle_sdiv(instruction);
    case Opcode::Udiv: return handle_udiv(instruction);
    case Opcode::Srem: return handle_srem(instruction);
    case Opcode::Shl: return handle_shl(instruction);
    case Opcode::Ashr: return handle_ashr(instruction);
    case Opcode::Or: return handle_or(instruction);
    case Opcode::Sext32: return handle_sext32(instruction);
    case Opcode::Abs: return handle_abs(instruction);
    case Opcode::Umax: return handle_umax(instruction);
    case Opcode::Clamp8: return handle_clamp8(instruction);
    case Opcode::Mdiv: return handle_mdiv(instruction);
    case Opcode::Alloc: return handle_alloc(instruction);
    case Opcode::Ld: return handle_ld(instruction);
    case Opcode::St: return handle_st(instruction);
    case Opcode::Jmp: return handle_jmp(instruction);
    case Opcode::Bcmp: return handle_bcmp(instruction);
    case Opcode::Brect: return handle_brect(instruction);
    case Opcode::Call: return handle_call(instruction);
    case Opcode::Ret: return handle_ret(instruction);
    case Opcode::Init: return handle_init(instruction);
    case Opcode::Exit: return handle_exit(instruction);
    case Opcode::Flush: return handle_flush(instruction);
    case Opcode::Putpixel: return handle_putpixel(instruction);
    case Opcode::Rand: return handle_rand(instruction);
    case Opcode::Hasclick: return handle_hasclick(instruction);
    case Opcode::Getclick: return handle_getclick(instruction);
    }
    fail("unknown opcode");
}

void Simulator::handle_mov(const Instruction& instruction) {
    write(instruction, read(instruction.operands[1]));
}

void Simulator::handle_add(const Instruction& instruction) {
    write(instruction, (read(instruction.operands[1]) + read(instruction.operands[2])) &
                       mask(instruction.width));
}

void Simulator::handle_sub(const Instruction& instruction) {
    write(instruction, (read(instruction.operands[1]) - read(instruction.operands[2])) &
                       mask(instruction.width));
}

void Simulator::handle_mul(const Instruction& instruction) {
    write(instruction, (read(instruction.operands[1]) * read(instruction.operands[2])) &
                       mask(instruction.width));
}

void Simulator::handle_sdiv(const Instruction& instruction) {
    write(instruction, divide(read(instruction.operands[1]), read(instruction.operands[2]),
                              instruction.width, false));
}

void Simulator::handle_udiv(const Instruction& instruction) {
    const auto divisor = read(instruction.operands[2]) & mask(instruction.width);
    if (divisor == 0) {
        fail("division by zero");
    }
    write(instruction, (read(instruction.operands[1]) & mask(instruction.width)) / divisor);
}

void Simulator::handle_srem(const Instruction& instruction) {
    write(instruction, divide(read(instruction.operands[1]), read(instruction.operands[2]),
                              instruction.width, true));
}

void Simulator::handle_shl(const Instruction& instruction) {
    const auto count = read(instruction.operands[2]) & mask(instruction.width);
    if (count >= instruction.width) {
        fail("shift count is outside the instruction width");
    }
    write(instruction, (read(instruction.operands[1]) << count) & mask(instruction.width));
}

void Simulator::handle_ashr(const Instruction& instruction) {
    const auto count = read(instruction.operands[2]) & mask(instruction.width);
    if (count >= instruction.width) {
        fail("shift count is outside the instruction width");
    }
    const auto value = read(instruction.operands[1]) & mask(instruction.width);
    auto result = value >> count;
    if (count != 0 && signed_value(value, instruction.width) < 0) {
        result |= mask(instruction.width) ^ (mask(instruction.width) >> count);
    }
    write(instruction, result);
}

void Simulator::handle_or(const Instruction& instruction) {
    write(instruction, (read(instruction.operands[1]) | read(instruction.operands[2])) &
                       mask(instruction.width));
}

void Simulator::handle_sext32(const Instruction& instruction) {
    write(instruction, static_cast<std::uint64_t>(signed_value(read(instruction.operands[1]), 32)));
}

void Simulator::handle_abs(const Instruction& instruction) {
    const auto value = read(instruction.operands[1]);
    if (value == (std::uint64_t{1} << 63)) {
        fail("abs of -2^63 is not representable");
    }
    write(instruction, signed_value(value) < 0 ? std::uint64_t{0} - value : value);
}

void Simulator::handle_umax(const Instruction& instruction) {
    write(instruction, std::max(read(instruction.operands[1]), read(instruction.operands[2])));
}

void Simulator::handle_clamp8(const Instruction& instruction) {
    const auto value = signed_value(read(instruction.operands[1]));
    write(instruction, static_cast<std::uint64_t>(std::clamp<std::int64_t>(value, 0, 255)));
}

void Simulator::handle_mdiv(const Instruction& instruction) {
    const auto product = (read(instruction.operands[0]) * read(instruction.operands[1])) &
                         mask(instruction.width);
    write(instruction, divide(product, read(instruction.operands[2]), instruction.width, false));
}

void Simulator::handle_alloc(const Instruction& instruction) {
    const auto count = read(instruction.operands[1]);
    if (next_word_ > memory_.size() || count > memory_.size() - next_word_) {
        fail("simulated memory exhausted");
    }
    const auto begin = next_word_;
    const auto end = begin + static_cast<std::size_t>(count);
    allocations_.push_back({begin, end});
    next_word_ = end + end % 2;
    write(instruction, begin * sizeof(std::uint64_t));
}

void Simulator::handle_ld(const Instruction& instruction) {
    const auto index = memory_index(read(instruction.operands[1]), read(instruction.operands[2]));
    if (!initialized_[index]) {
        fail("read of an uninitialized memory word");
    }
    write(instruction, memory_[index]);
}

void Simulator::handle_st(const Instruction& instruction) {
    const auto index = memory_index(read(instruction.operands[0]), read(instruction.operands[1]));
    memory_[index] = read(instruction.operands[2]);
    initialized_[index] = true;
}

void Simulator::handle_jmp(const Instruction& instruction) {
    pc_ = target(instruction.operands[0]);
}

void Simulator::handle_bcmp(const Instruction& instruction) {
    const auto a = read(instruction.operands[0]) & mask(instruction.width);
    const auto b = read(instruction.operands[1]) & mask(instruction.width);
    bool matches = false;
    switch (instruction.predicate) {
    case Predicate::Eq: matches = a == b; break;
    case Predicate::Slt: matches = signed_value(a, instruction.width) < signed_value(b, instruction.width); break;
    case Predicate::Sgt: matches = signed_value(a, instruction.width) > signed_value(b, instruction.width); break;
    case Predicate::None: fail("comparison has no predicate");
    }
    if (matches) {
        pc_ = target(instruction.operands[2]);
    }
}

void Simulator::handle_brect(const Instruction& instruction) {
    if (read(instruction.operands[0]) < SIM_X_SIZE && read(instruction.operands[1]) < SIM_Y_SIZE) {
        pc_ = target(instruction.operands[2]);
    }
}

void Simulator::handle_call(const Instruction& instruction) {
    const auto destination = target(instruction.operands[0]);
    return_stack_.push_back(pc_);
    pc_ = destination;
}

void Simulator::handle_ret(const Instruction&) {
    if (return_stack_.empty()) {
        halted_ = true;
        return;
    }
    pc_ = return_stack_.back();
    return_stack_.pop_back();
}

void Simulator::handle_init(const Instruction&) {
    ensure_sim();
}

void Simulator::handle_exit(const Instruction&) {
    close_sim();
}

void Simulator::handle_flush(const Instruction&) {
    ensure_sim();
    simFlush();
}

void Simulator::handle_putpixel(const Instruction& instruction) {
    const auto x = signed_value(read(instruction.operands[0]), 32);
    const auto y = signed_value(read(instruction.operands[1]), 32);
    const auto color = signed_value(read(instruction.operands[2]), 32);
    if (x < 0 || x >= SIM_X_SIZE || y < 0 || y >= SIM_Y_SIZE) {
        fail("pixel coordinates are outside the screen");
    }
    ensure_sim();
    simPutPixel(static_cast<int>(x), static_cast<int>(y), static_cast<int>(color));
}

void Simulator::handle_rand(const Instruction& instruction) {
    ensure_sim();
    write(instruction, static_cast<std::uint32_t>(simRand()));
}

void Simulator::handle_hasclick(const Instruction& instruction) {
    ensure_sim();
    write(instruction, static_cast<std::uint32_t>(simHasClick()));
}

void Simulator::handle_getclick(const Instruction& instruction) {
    ensure_sim();
    write(instruction, static_cast<std::uint32_t>(simGetClick()));
}

} // namespace warp
