#include <iostream>
#include <string>

#include "../Algorithms/RAPTOR/ULTRARAPTOR.h"

#include "../DataStructures/RAPTOR/Data.h"
#include "../Custom/json.h"

inline void usage() noexcept {
    std::cout << "Usage: ultra-server  <RAPTOR binary>  <TODO>" << std::endl;
    exit(0);
}

using ShortcutRAPTOR = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

int main(int argc, char** argv) {
    if (argc < 3)
        usage();
    const std::string raptorFile = argv[1];
    const std::string bucketChBasename = argv[2];

    std::cout << "raptorFile       = " << raptorFile << std::endl;
    std::cout << "bucketChBasename = " << raptorFile << std::endl;

    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();

    CH::CH bucketCH(bucketChBasename);
    ShortcutRAPTOR algorithm(data, bucketCH);

    /* int SOURCE = 435; */
    /* int TARGET = 120; */
    int SOURCE = 4350;
    int TARGET = 1200;
    int DEPARTURE_TIME = 36000;

    if (!data.isStop(Vertex{SOURCE}) || !data.isStop(Vertex{TARGET})) {
        std::cout << "For now, we can only handle journeys between stops :" << std::endl;
        std::cout << "Is SOURCE a stop ? " << data.isStop(Vertex{SOURCE}) << std::endl;
        std::cout << "Is TARGET a stop ? " << data.isStop(Vertex{TARGET}) << std::endl;
        return 1;
    }

    auto legs = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET));
    std::cout << std::endl;
    std::cout << "JOURNEY = " << SOURCE << "   -->   " << TARGET << std::endl;
    std::cout << "LEGS" << std::endl;
    for (auto leg : legs) {
        std::cout << leg.as_string() << std::endl;
    }
    std::cout << std::endl;

    return 0;
}
