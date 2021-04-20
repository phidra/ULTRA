#pragma once

#include <vector>
#include <filesystem>

#include "Graph/graphtypes.h"
#include "Graph/polygon.h"

namespace my::preprocess {

struct WalkingGraph {

    // From a set of stops and a given OSM file (+ a possible filtering polygon), computes a walking graph.
    //
    // The graph is extended with new nodes (the stops) and new edges (between each stop and its closest OSM node).
    // Also : the edges are duplicated to make the graph bidirectional.
    //
    // Each node of the graph is identified by its rank.
    // Nodes representing stops are ranked BEFORE the other nodes (because this is needed by ULTRA).

    WalkingGraph(
        std::filesystem::path osmFile,
        std::filesystem::path polygonFile,
        std::vector<my::Stop> const& stops,
        float walkspeedKmPerHour_
    );

    float walkspeedKmPerHour;
    my::BgPolygon polygon;

    // edges1 = those are the "initial" edges, in the OSM graph :
    std::vector<my::Edge> edgesOsm;

    // edges2 = those are the edges "augmented" with an edge between each stop and its closest initial node :
    std::vector<my::Edge> edgesWithStops;

    // edges3 = same than edges2, but twice as more because bidirectional :
    std::vector<my::Edge> edgesWithStopsBidirectional;


    // those are the stops passed as parameters, augmented with their closest node in the OSM graph :
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;

    // helper structures :
    std::unordered_map<my::NodeId, size_t> nodeToRank;
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;

    // this dump helper is currently used to check non-regression (output must be binary iso)
    // later, we can remove it or replace it with proper tests, but for now it is useful
    void dumpIntermediary(std::string const& outputDir) const;
};

}

