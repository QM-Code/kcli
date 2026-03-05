#include <kcli.hpp>

#include <iostream>

int main(int argc, char** argv) {
    kcli::Parser cli;

    // Grabs argc and argv for future use.
    cli.Initialize(argc, argv);
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}
