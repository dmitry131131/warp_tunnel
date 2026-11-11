#pragma once

#include "../SIM/simulator.h"

#include <exception>
#include <filesystem>
#include <vector>

namespace llvm {
class ExecutionEngine;
class Module;
}

namespace sim {

enum class JitMode { HandlerCalls, NativeIR };

class JitRunner {
public:
    explicit JitRunner(const Program& program, std::size_t memory_words = 1024 * 1024);
    void run(JitMode mode, const std::filesystem::path& ir_output = {});
    const std::array<std::uint64_t, register_count>& registers() const {
        return machine_.registers();
    }

private:
    Simulator machine_;
    std::exception_ptr error_;
    std::vector<std::uint8_t> initialized_;
    std::vector<std::uint64_t> allocation_begin_;
    std::vector<std::uint64_t> allocation_end_;
    std::uint8_t sim_ready_ = 0;

    using Handler = void (Simulator::*)(const Instruction&);
    template<Handler handler>
    static bool invoke(void* context, std::uint64_t index) noexcept {
        auto& runner = *static_cast<JitRunner*>(context);
        try {
            const auto& instruction = runner.machine_.program_.instructions.at(index);
            runner.machine_.source_line_ = instruction.source_line;
            (runner.machine_.*handler)(instruction);
            return true;
        } catch (...) {
            // Exceptions never cross generated machine code.
            runner.error_ = std::current_exception();
            return false;
        }
    }

    static void report_error(void* context, std::uint64_t line, std::uint64_t code) noexcept;
    void map_symbols(llvm::ExecutionEngine& engine, llvm::Module& module);
    void close_runtime();
};

} // namespace sim
