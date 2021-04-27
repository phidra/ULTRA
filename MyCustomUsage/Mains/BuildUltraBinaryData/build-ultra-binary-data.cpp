#include <iostream>
#include <string>

#include "Preprocess/ultra_transfer_data.h"
#include "Preprocess/ultra_gtfs_data.h"

#include "MutualizedPreprocess/GtfsParsing/gtfs_parsed_data.h"
#include "MutualizedPreprocess/GtfsParsing/gtfs_geojson.h"

#include "DataStructures/RAPTOR/Data.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName << "  <prepared GTFS>  <walking-graph>  <outputDir>" << std::endl;
    exit(0);
}

my::preprocess::UltraTransferData buildTransferData(my::preprocess::WalkingGraph&& graph, std::string programName) {
    try {
        my::preprocess::UltraTransferData transferData{std::move(graph)};
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
    if (argc < 4)
        usage(argv[0]);

    const std::string preparatoryGtfsFile = argv[1];
    const std::string preparatoryGraph = argv[2];
    std::string outputDir = argv[3];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "PREPARATORY GTFS  = " << preparatoryGtfsFile << std::endl;
    std::cout << "PREPARATORY GRAPH = " << preparatoryGraph << std::endl;
    std::cout << "OUTPUT_DIR        = " << outputDir << std::endl;
    std::cout << std::endl;

    std::ifstream gtfs_input_stream{preparatoryGtfsFile};
    my::preprocess::GtfsParsedData gtfs = my::preprocess::fromStream(gtfs_input_stream);
    my::preprocess::UltraGtfsData gtfsData{gtfs};

    std::ifstream walking_graph_input_stream{preparatoryGraph};
    my::preprocess::WalkingGraph graph = my::preprocess::WalkingGraph::fromStream(walking_graph_input_stream);

    my::preprocess::UltraTransferData transferData = buildTransferData(std::move(graph), argv[0]);

    transferData.walkingGraph.printStats(std::cout);
    std::cout << "transferGraph vertices : " << transferData.transferGraphUltra.numVertices() << std::endl;
    std::cout << "transferGraph edges    : " << transferData.transferGraphUltra.numEdges() << std::endl;

    const std::filesystem::path intermediaryDir = outputDir + "INTERMEDIARY/";
    std::filesystem::create_directory(intermediaryDir);
    // FIXME : these intermediary structures should be in the preprocess :
    // transferData.walkingGraph.dumpIntermediary(intermediaryDir);

    // serializing data like RAPTOR::Data does :
    const std::string raptorDataFileName = outputDir + "raptor.binary";
    gtfsData.serialize(raptorDataFileName);
    transferData.transferGraphUltra.writeBinary(raptorDataFileName + ".graph");

    // checking that we can unserialize it, and find no mismatch :
    auto unserialized = RAPTOR::Data::FromBinary(raptorDataFileName);
    bool areApproxEqual =
        my::preprocess::UltraTransferData::areApproxEqual(transferData.transferGraphUltra, unserialized.transferGraph);
    std::cout << "La serialization est-elle idempotente pour le transferGraph ? " << std::boolalpha << areApproxEqual
              << std::endl;

    return 0;
}
