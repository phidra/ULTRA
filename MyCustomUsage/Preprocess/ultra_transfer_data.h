#pragma once

#include <vector>
#include <filesystem>

#include "DataStructures/Graph/Graph.h"
#include "DataStructures/RAPTOR/Entities/Stop.h"
#include "Common/graphtypes.h"

namespace my::preprocess {

struct UltraTransferData {

    UltraTransferData(
        std::filesystem::path osmFile,
        std::filesystem::path polygonFile,
        std::vector<RAPTOR::Stop> const& stops,
        float walkspeedKmPerHour
    );
    void dumpIntermediary(std::string const& outputDir) const;
    static bool areApproxEqual(TransferGraph const& left, TransferGraph const& right);
    bool checkSerializationIdempotence() const;

    TransferGraph transferGraph;

    // intermediate data :
    std::vector<my::Stop> stops;
    std::vector<my::Edge> edges;
    std::vector<my::Edge> edgesWithStops;
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;
};

}
