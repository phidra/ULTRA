#include <iostream>
#include <map>

#include "walking_graph.h"

#include "Preprocess/Parsing/polygonfile.h"
#include "Preprocess/Graph/extending_with_stops.h"
#include "Preprocess/Graph/graph.h"


namespace my::preprocess {

std::unordered_map<my::NodeId, size_t> _rankNodes(std::vector<my::Edge> const& edgesWithStops, std::vector<my::Stop> const& stops) {
    std::unordered_map<my::NodeId, size_t> nodeToRank;

    // some algorithms (ULTRA) need that stops are the first nodes of the graph -> stops must be ranked first :
    size_t rank = 0;
    std::for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rank](my::Stop const& stop) {
        nodeToRank.insert({stop.id, rank++});
    });

    auto rankThatNode = [&nodeToRank, &rank](auto const& node_id) {
        if (nodeToRank.find(node_id) == nodeToRank.end()) {
            nodeToRank.insert({node_id, rank++});
        }
    };

    // then we can rank the other nodes in the graph :
    for (auto edge: edgesWithStops) {
        rankThatNode(edge.node_from.id);
        rankThatNode(edge.node_to.id);
    }

    return nodeToRank;
}

std::vector<my::Edge> _makeEdgesBidirectional(std::vector<my::Edge> const& edges) {
    // For each edge, adds its opposite edge (this doubles the number of edges in the edgelist)
    // note : the function is cleaner without side-effects (taking a const ref to edges, returning a copy)
    // but as this is slower, if performance issues arise, we can mutate edges instead
    std::vector<my::Edge> bidirectional(edges);
    for (auto edge: edges) {
        Polyline geom = edge.geometry;
        std::reverse(geom.begin(), geom.end());
        bidirectional.emplace_back(
            edge.node_to.id,
            edge.node_from.id,
            std::move(geom),
            edge.length_m,
            edge.weight
        );
    }
    assert(bidirectional.size() == 2 * edges.size());
    return bidirectional;
}

std::map<size_t, std::vector<size_t>> _mapNodesToOutEdges(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // this functions build a map that helps to retrieve the out-edges of a node (given its rank)
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;
    for (size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
        auto const& edge = edges[edge_index];
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        nodeToOutEdges[node_from_rank].push_back(edge_index);
    }
    return nodeToOutEdges;
}

WalkingGraph::WalkingGraph(
    std::filesystem::path osmFile,
    std::filesystem::path polygonFile,
    std::vector<my::Stop> const& stops,
    float walkspeedKmPerHour_) :
    walkspeedKmPerHour{walkspeedKmPerHour_},
    polygon{get_polygon(polygonFile)},
    edgesOsm{osm_to_graph(osmFile, polygon, walkspeedKmPerHour)} {

    // extend graph with stop-edges :
    std::tie(edgesWithStops, stopsWithClosestNode) = extend_graph(stops, edgesOsm, walkspeedKmPerHour);

    nodeToRank = _rankNodes(edgesWithStops, stops);
    std::cout << "nb ranked nodes = " << nodeToRank.size() << std::endl;

    edgesWithStopsBidirectional = _makeEdgesBidirectional(edgesWithStops);
    nodeToOutEdges = _mapNodesToOutEdges(edgesWithStopsBidirectional, nodeToRank);
    std::cout << "The association map has " << nodeToOutEdges.size() << " items" << std::endl;
}


}

