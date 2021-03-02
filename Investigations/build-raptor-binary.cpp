#include <iostream>
#include <string>

#include "../Custom/transfer_graph.h"
#include "../Custom/gtfs_to_ultra_binary.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName << "  <GTFS folder>  <osmFile>  <polygonFile>  <gtfsStopFile>  <outputDir>" << std::endl;
    exit(0);
}

my::UltraTransferData buildTransferData(
    std::filesystem::path osmFile,
    std::filesystem::path polygonFile,
    std::filesystem::path gtfsStopFile,
    float walkspeedKmPerHour,
    std::string programName) {
    try {
        my::UltraTransferData transferData{osmFile, polygonFile, gtfsStopFile, walkspeedKmPerHour};
        return transferData;
    } catch (std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        usage(programName);
        exit(2);
    } catch (...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
        usage(programName);
        exit(2);
    }
}


int main(int argc, char** argv) {
    if (argc < 6)
        usage(argv[0]);

    const std::string gtfsFolder = argv[1];
    const std::string osmFile = argv[2];
    const std::string polygonFile = argv[3];
    auto gtfsStopFile = argv[4];
    std::string outputDir = argv[5];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "GTFS FOLDER      = " << gtfsFolder << std::endl;
    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
    std::cout << "STOPFILE         = " << gtfsStopFile << std::endl;
    std::cout << "OUTPUT_DIR       = " << outputDir << std::endl;
    std::cout << std::endl;

    constexpr const float walkspeedKmPerHour = 4.7;

    my::UltraTransferData transferData = buildTransferData(
        osmFile,
        polygonFile,
        gtfsStopFile,
        walkspeedKmPerHour,
        argv[0]);

    std::cout << "Number of edges in original graph : " << transferData.edges.size() << std::endl;
    std::cout << "nb edges (including added stops) = " << transferData.edgesWithStops.size() << std::endl;
    std::cout << "nb stops = " << transferData.stopsWithClosestNode.size() << std::endl;
    std::cout << "The transferGraph has these vertices : " << transferData.transferGraph.numVertices() << std::endl;
    std::cout << "The transferGraph has these edges    : " << transferData.transferGraph.numEdges() << std::endl;


    my::UltraGtfsData binaryData{gtfsFolder};

    return 0;
}
