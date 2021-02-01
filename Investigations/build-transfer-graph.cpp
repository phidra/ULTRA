#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"

#ifndef BINARY_NAME
#define BINARY_NAME binary
#endif

inline void usage() noexcept {
    std::cout << "Usage: ./BINARY_NAME  <osmfile>  <polygonfile>" << std::endl;
    exit(0);
}

int main(int argc, char** argv) {
    if (argc < 3)
        usage();

    const std::string osmfile = argv[1];
    const std::string polygonfile = argv[2];

    std::cout << "osmfile          = " << osmfile << std::endl;
    std::cout << "polygonfile      = " << polygonfile << std::endl;
    std::cout << std::endl;

    return 0;
}
