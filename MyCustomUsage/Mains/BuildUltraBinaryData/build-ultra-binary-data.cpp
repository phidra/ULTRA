#include <iostream>
#include <string>

#include "Preprocess/ultra_transfer_data.h"
#include "Preprocess/ultra_gtfs_data.h"

#include "MutualizedPreprocess/GtfsParsing/gtfs_parsed_data.h"
#include "MutualizedPreprocess/GtfsParsing/gtfs_geojson.h"

#include "DataStructures/RAPTOR/Data.h"

inline void usage(const std::string programName) noexcept {
    std::cout << "Usage:  " << programName
              << "  <preparatory GTFS>  <osmFile>  <polygonFile>  <walkspeed km/h>  <outputDir>" << std::endl;
    exit(0);
}

my::preprocess::UltraTransferData buildTransferData(std::filesystem::path osmFile,
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
    if (argc < 6)
        usage(argv[0]);

    const std::string preparatoryGtfsFile = argv[1];
    const std::string osmFile = argv[2];
    const std::string polygonFile = argv[3];
    const float walkspeedKmPerHour = std::stof(argv[4]);
    std::string outputDir = argv[5];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::cout << "PREPARATORY GTFS = " << preparatoryGtfsFile << std::endl;
    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
    std::cout << "WALKSPEED KM/H   = " << walkspeedKmPerHour << std::endl;
    std::cout << "OUTPUT_DIR       = " << outputDir << std::endl;
    std::cout << std::endl;

    std::ifstream gtfs_input_stream{preparatoryGtfsFile};
    my::preprocess::GtfsParsedData gtfs = my::preprocess::fromStream(gtfs_input_stream);
    my::preprocess::UltraGtfsData gtfsData{gtfs};

    my::preprocess::UltraTransferData transferData =
        buildTransferData(osmFile, polygonFile, gtfsData.stopData, walkspeedKmPerHour, argv[0]);

    transferData.walkingGraph.printStats(std::cout);
    std::cout << "The transferGraph has these vertices : " << transferData.transferGraphUltra.numVertices()
              << std::endl;
    std::cout << "The transferGraph has these edges    : " << transferData.transferGraphUltra.numEdges() << std::endl;

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
