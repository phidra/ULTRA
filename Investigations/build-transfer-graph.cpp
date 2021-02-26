#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"
#include "../DataStructures/Geometry/Point.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Parsing/gtfs_stops.h"
#include "../Custom/Graph/extending_with_stops.h"
#include "../Custom/Graph/graph.h"
#include "../Custom/transfer_graph.h"
#include "../Custom/Dumping/geojson.h"

inline void usage(const std::string binary_name) noexcept {
    std::cout << "Usage:  " << binary_name << "  <osmFile>  <polygonFile>  <gtfs-stopfile>  <output-dir>" << std::endl;
    exit(0);
}


int main(int argc, char** argv) {
    if (argc < 5)
        usage(argv[0]);

    const std::string osmFile = argv[1];
    const std::string polygonFile = argv[2];
    auto gtfsStopFile = argv[3];
    std::string outputDir = argv[4];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
    std::cout << "STOPFILE         = " << gtfsStopFile << std::endl;
    std::cout << "OUTPUT_DIR       = " << outputDir << std::endl;
    std::cout << std::endl;

    constexpr const float walkspeedKmPerHour = 4.7;

    TransferGraph transferGraph;
    try {
        my::UltraTransferData transferData{osmFile, polygonFile, gtfsStopFile, walkspeedKmPerHour};
        transferData.dumpIntermediary(outputDir);
        transferGraph = std::move(transferData.transferGraph);
    } catch (std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        usage(argv[0]);
        exit(2);
    } catch (...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
        usage(argv[0]);
        exit(2);
    }

    // serializing :
    const std::string outputFileName = outputDir + "serialized.binary.graph";
    transferGraph.writeBinary(outputFileName);
    std::cout << "TransferGraph dumped in : " << outputFileName << std::endl;

    // unserializing, to see if serialization+serialization is idempotent :
    TransferGraph freshTransferGraph;
    freshTransferGraph.readBinary(outputFileName);

    std::ostringstream stats1_stream;
    transferGraph.printAnalysis(stats1_stream);
    const std::string stats1 = stats1_stream.str();

    std::ostringstream stats2_stream;
    freshTransferGraph.printAnalysis(stats2_stream);
    const std::string stats2 = stats2_stream.str();

    std::cout << "Is serialization+deserialization idempotent ?" << std::endl;
    std::cout << (stats1 == stats2) << std::endl;
    std::cout << stats1 << std::endl;

    return 0;
}
