#include "parser.h"
#include "simulator.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: warp_sim <program.asm>\n";
        return 1;
    }

    try {
        const auto program = sim::parse_file(argv[1]);
        std::cout << "Parsed " << program.instructions.size() << " instructions, "
                  << program.labels.size() << " labels, entry instruction "
                  << program.entry_point << '\n';
        sim::Simulator simulator(program);
        simulator.run();
    } catch (const sim::ExecutionError& error) {
        std::cerr << argv[1] << ':' << error.line() << ": " << error.what() << '\n';
        return 1;
    } catch (const sim::ParseError& error) {
        std::cerr << argv[1] << ':' << error.line() << ": " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << argv[1] << ": " << error.what() << '\n';
        return 1;
    }
}
