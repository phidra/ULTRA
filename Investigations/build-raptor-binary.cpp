#include <iostream>
#include <string>

#include "../Custom/transfer_graph.h"
#include "../Custom/gtfs_to_ultra_binary.h"
#include "../DataStructures/RAPTOR/Data.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName << "  <GTFS folder>  <osmFile>  <polygonFile>  <outputDir>" << std::endl;
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
    if (argc < 5)
        usage(argv[0]);

    const std::string gtfsFolder = argv[1];
    const std::string osmFile = argv[2];
    const std::string polygonFile = argv[3];
    std::string outputDir = argv[4];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }
    auto gtfsStopFile = gtfsFolder + (gtfsFolder.back() != '/' ? "/stops.txt" : "stops.txt");

    std::cout << "GTFS FOLDER      = " << gtfsFolder << std::endl;
    std::cout << "GTFS STOPFILE    = " << gtfsStopFile << std::endl;
    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
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

    transferData.dumpIntermediary(outputDir + "INTERMEDIARY/");

    my::UltraGtfsData binaryData{gtfsFolder};

    // serializing data like RAPTOR::Data does :
    const std::string raptorDataFileName = outputDir + "raptor.binary";

    // For the record, here is the serialization code in RAPTOR::Data :
    /* inline void serialize(const std::string& fileName) const noexcept { */
    /*     IO::serialize(fileName, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes); */
    /*     transferGraph.writeBinary(fileName + ".graph"); */
    /* } */
    binaryData.serialize(raptorDataFileName);
    transferData.transferGraph.writeBinary(raptorDataFileName + ".graph");

    // checking that we can unserialize it, and find no mismatch :
    auto unserialized = RAPTOR::Data::FromBinary(raptorDataFileName);
    bool areApproxEqual = my::UltraTransferData::areApproxEqual(transferData.transferGraph, unserialized.transferGraph);
    std::cout << "La serialization est-elle idempotente pour le transferGraph ? " << std::boolalpha << areApproxEqual << std::endl;

    return 0;
}
