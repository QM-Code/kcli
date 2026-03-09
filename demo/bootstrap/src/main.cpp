#include <kcli.hpp>

#include <iostream>

int main(int argc, char** argv) {
    kcli::PrimaryParser parser;
    parser.parse(argc, argv);
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}
