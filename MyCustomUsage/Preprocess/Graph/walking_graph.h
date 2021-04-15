#pragma once

#include <vector>
#include <filesystem>

#include "Common/graphtypes.h"
#include "Common/polygon.h"

namespace my::preprocess {

struct WalkingGraph {

    WalkingGraph(
        std::filesystem::path osmFile,
        std::filesystem::path polygonFile,
        std::vector<my::Stop> const& stops,
        float walkspeedKmPerHour_
    );

    float walkspeedKmPerHour;
    my::BgPolygon polygon;

    std::vector<my::Stop> stops;
    std::vector<my::Edge> edges;
    std::vector<my::Edge> edgesWithStops;
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;

    std::vector<my::Edge> bidirectionalEdges;
    std::unordered_map<my::NodeId, size_t> nodeToRank;
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;
};

}

