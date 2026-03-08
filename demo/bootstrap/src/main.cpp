#include <kcli.hpp>

#include <iostream>

int main(int argc, char** argv) {
    // Grabs argc and argv for future use.
    kcli::Initialize(argc, argv);
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}
