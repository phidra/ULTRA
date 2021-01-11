#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"

inline void usage() noexcept {
    std::cout << "Usage: binary-analyzer <RAPTOR binary>" << std::endl;
    exit(0);
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();
    const std::string raptorFile = argv[1];
    std::cout << "Analyzing : " << raptorFile << std::endl;
    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();
    return 0;
}
