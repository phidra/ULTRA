#include <iostream>
#include <string>

#include "../Custom/Preprocess/transfer_graph.h"
#include "../Custom/Preprocess/ultra_gtfs_data.h"
#include "../DataStructures/RAPTOR/Data.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName << "  <GTFS folder>  <osmFile>  <polygonFile>  <outputDir>" << std::endl;
    exit(0);
}

my::preprocess::UltraTransferData buildTransferData(
    std::filesystem::path osmFile,
    std::filesystem::path polygonFile,
    std::vector<RAPTOR::Stop> const& stopData,
    float walkspeedKmPerHour,
    std::string programName) {
    try {
        my::preprocess::UltraTransferData transferData{osmFile, polygonFile, stopData, walkspeedKmPerHour};
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
    if (argc < 5)
        usage(argv[0]);

    const std::string gtfsFolder = argv[1];
    const std::string osmFile = argv[2];
    const std::string polygonFile = argv[3];
    std::string outputDir = argv[4];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "GTFS FOLDER      = " << gtfsFolder << std::endl;
    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
    std::cout << "OUTPUT_DIR       = " << outputDir << std::endl;
    std::cout << std::endl;

    constexpr const float walkspeedKmPerHour = 4.7;
    my::preprocess::UltraGtfsData binaryData{gtfsFolder};


    my::preprocess::UltraTransferData transferData = buildTransferData(
        osmFile,
        polygonFile,
        binaryData.stopData,
        walkspeedKmPerHour,
        argv[0]);

    std::cout << "Number of edges in original graph : " << transferData.edges.size() << std::endl;
    std::cout << "nb edges (including added stops) = " << transferData.edgesWithStops.size() << std::endl;
    std::cout << "nb stops = " << transferData.stopsWithClosestNode.size() << std::endl;
    std::cout << "The transferGraph has these vertices : " << transferData.transferGraph.numVertices() << std::endl;
    std::cout << "The transferGraph has these edges    : " << transferData.transferGraph.numEdges() << std::endl;

    const std::filesystem::path intermediaryDir = outputDir + "INTERMEDIARY/";
    std::filesystem::create_directory(intermediaryDir);
    transferData.dumpIntermediary(intermediaryDir);


    // serializing data like RAPTOR::Data does :
    const std::string raptorDataFileName = outputDir + "raptor.binary";
    binaryData.serialize(raptorDataFileName);
    transferData.transferGraph.writeBinary(raptorDataFileName + ".graph");

    // checking that we can unserialize it, and find no mismatch :
    auto unserialized = RAPTOR::Data::FromBinary(raptorDataFileName);
    bool areApproxEqual = my::preprocess::UltraTransferData::areApproxEqual(transferData.transferGraph, unserialized.transferGraph);
    std::cout << "La serialization est-elle idempotente pour le transferGraph ? " << std::boolalpha << areApproxEqual << std::endl;

    return 0;
}
