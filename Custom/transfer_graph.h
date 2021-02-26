#pragma once

#include <vector>
#include <filesystem>

#include "../DataStructures/Graph/Graph.h"
#include "Common/graphtypes.h"

namespace my {

TransferGraph buildTransferGraph(
    std::vector<my::Edge> const& edgesWithStops,
    std::vector<my::StopWithClosestNode> const& stopsWithClosestNode);

struct UltraTransferData {

    UltraTransferData(std::filesystem::path osmFile, std::filesystem::path polygonFile, std::filesystem::path gtfsStopfile, float walkspeedKmPerHour_);

    TransferGraph transferGraph;

private:
    std::vector<my::Stop> stops;
    float walkspeedKmPerHour;
    std::vector<my::Edge> edges;
    std::vector<my::Edge> edgesWithStops;
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;
};

}
