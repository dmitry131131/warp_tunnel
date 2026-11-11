#pragma once

#include "parser.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace warp {

class ExecutionError : public std::runtime_error {
public:
    ExecutionError(std::size_t line, const std::string& message);
    std::size_t line() const noexcept { return line_; }

private:
    std::size_t line_;
};

class Simulator {
public:
    // Memory capacity is measured in 64-bit words. Addresses in registers are
    // byte offsets into this array, never pointers into the host process.
    explicit Simulator(const Program& program, std::size_t memory_words = 1024 * 1024);
    void run();
    const std::array<std::uint64_t, register_count>& registers() const { return registers_; }

private:
    struct Allocation {
        std::size_t begin;
        std::size_t end;
    };

    const Program& program_;
    std::array<std::uint64_t, register_count> registers_{};
    std::vector<std::uint64_t> memory_;
    std::vector<bool> initialized_;
    std::vector<Allocation> allocations_;
    std::vector<std::size_t> return_stack_;
    std::size_t next_word_ = 2; // Address zero is reserved, allocations align to 16 bytes.
    std::size_t pc_ = 0;
    std::size_t source_line_ = 0;
    bool halted_ = false;
    bool sim_initialized_ = false;

    [[noreturn]] void fail(const std::string& message) const;
    std::uint64_t read(const Operand& operand) const;
    void write(const Instruction& instruction, std::uint64_t value);
    std::size_t target(const Operand& operand) const;
    std::size_t memory_index(std::uint64_t base, std::uint64_t index) const;
    std::uint64_t divide(std::uint64_t a, std::uint64_t b, unsigned width,
                         bool remainder) const;
    void ensure_sim();
    void close_sim();
    void dispatch(const Instruction& instruction);

    void handle_mov(const Instruction&);
    void handle_add(const Instruction&);
    void handle_sub(const Instruction&);
    void handle_mul(const Instruction&);
    void handle_sdiv(const Instruction&);
    void handle_udiv(const Instruction&);
    void handle_srem(const Instruction&);
    void handle_shl(const Instruction&);
    void handle_ashr(const Instruction&);
    void handle_or(const Instruction&);
    void handle_sext32(const Instruction&);
    void handle_abs(const Instruction&);
    void handle_umax(const Instruction&);
    void handle_clamp8(const Instruction&);
    void handle_mdiv(const Instruction&);
    void handle_alloc(const Instruction&);
    void handle_ld(const Instruction&);
    void handle_st(const Instruction&);
    void handle_jmp(const Instruction&);
    void handle_bcmp(const Instruction&);
    void handle_brect(const Instruction&);
    void handle_call(const Instruction&);
    void handle_ret(const Instruction&);
    void handle_init(const Instruction&);
    void handle_exit(const Instruction&);
    void handle_flush(const Instruction&);
    void handle_putpixel(const Instruction&);
    void handle_rand(const Instruction&);
    void handle_hasclick(const Instruction&);
    void handle_getclick(const Instruction&);
};

} // namespace warp
