#include "jit.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <program.asm> [output.ll]\n";
        return 1;
    }
    try {
        const auto program = sim::parse_file(argv[1]);
        sim::JitRunner runner(program);
#ifdef SIM_HANDLER_CALLS
        constexpr auto mode = sim::JitMode::HandlerCalls;
#else
        constexpr auto mode = sim::JitMode::NativeIR;
#endif
        runner.run(mode, argc == 3 ? argv[2] : "");
    } catch (const sim::ParseError& error) {
        std::cerr << argv[1] << ':' << error.line() << ": " << error.what() << '\n';
        return 1;
    } catch (const sim::ExecutionError& error) {
        std::cerr << argv[1] << ':' << error.line() << ": " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
