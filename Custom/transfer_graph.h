#pragma once

#include <vector>
#include <filesystem>

#include "../DataStructures/Graph/Graph.h"
#include "Common/graphtypes.h"
#include "Common/polygon.h"

namespace my {

TransferGraph buildTransferGraph(
    std::vector<my::Edge> const& edgesWithStops,
    std::vector<my::StopWithClosestNode> const& stopsWithClosestNode);

struct UltraTransferData {

    UltraTransferData(std::filesystem::path osmFile, std::filesystem::path polygonFile, std::filesystem::path gtfsStopfile, float walkspeedKmPerHour_);
    void dumpIntermediary(std::string const& outputDir) const;

    float walkspeedKmPerHour;
    my::BgPolygon polygon;

    TransferGraph transferGraph;

    // intermediate data :
    std::vector<my::Stop> stops;
    std::vector<my::Edge> edges;
    std::vector<my::Edge> edgesWithStops;
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;
};

}
