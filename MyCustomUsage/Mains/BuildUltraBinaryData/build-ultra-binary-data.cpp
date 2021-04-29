#include <iostream>
#include <string>

#include "Preprocess/ultra_transfer_data.h"
#include "Preprocess/ultra_gtfs_data.h"

#include "gtfs/gtfs_parsed_data.h"
#include "json/gtfs_serialization.h"
#include "json/walking_graph_serialization.h"

#include "DataStructures/RAPTOR/Data.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName << "  <uwpreprocessed GTFS>  <uwpreprocessed graph>  <outputDir>"
              << std::endl;
    exit(0);
}

my::preprocess::UltraTransferData buildTransferData(uwpreprocess::WalkingGraph&& graph, std::string programName) {
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

    const std::string uwpreprocessedGtfsFile = argv[1];
    const std::string uwpreprocessedGraph = argv[2];
    std::string outputDir = argv[3];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "UWPREPROCESSED GTFS  = " << uwpreprocessedGtfsFile << std::endl;
    std::cout << "UWPREPROCESSED GRAPH = " << uwpreprocessedGraph << std::endl;
    std::cout << "OUTPUT_DIR           = " << outputDir << std::endl;
    std::cout << std::endl;

    my::preprocess::UltraGtfsData gtfsData;
    {
        std::ifstream gtfs_input_stream{uwpreprocessedGtfsFile};
        uwpreprocess::GtfsParsedData gtfs = uwpreprocess::json::unserialize_gtfs(gtfs_input_stream);
        my::preprocess::UltraGtfsData tmp{gtfs};
        gtfsData = std::move(tmp);
    }

    std::ifstream walking_graph_input_stream{uwpreprocessedGraph};
    uwpreprocess::WalkingGraph graph = uwpreprocess::json::unserialize_walking_graph(walking_graph_input_stream);
    my::preprocess::UltraTransferData transferData = buildTransferData(std::move(graph), argv[0]);

    std::cout << "transferGraph vertices : " << transferData.transferGraphUltra.numVertices() << std::endl;
    std::cout << "transferGraph edges    : " << transferData.transferGraphUltra.numEdges() << std::endl;

    const std::filesystem::path intermediaryDir = outputDir + "INTERMEDIARY/";
    std::filesystem::create_directory(intermediaryDir);

    // serializing data like RAPTOR::Data does :
    const std::string raptorDataFileName = outputDir + "raptor.binary";
    gtfsData.serialize(raptorDataFileName);
    transferData.transferGraphUltra.writeBinary(raptorDataFileName + ".graph");

    return 0;
}
