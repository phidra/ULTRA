#include <iostream>
#include <map>

#include "walking_graph.h"

#include "Preprocess/Parsing/polygonfile.h"
#include "Preprocess/Graph/extending_with_stops.h"
#include "Preprocess/Graph/graph.h"


namespace my::preprocess {

std::pair<std::vector<my::NodeId>, std::unordered_map<my::NodeId, size_t>>
_rankNodes(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stops) {
    std::vector<my::NodeId> rankedNodes;
    std::unordered_map<my::NodeId, size_t> nodeToRank;

    // some algorithms (ULTRA) need that stops are the first nodes of the graph -> stops must be ranked first :
    std::for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rankedNodes](my::StopWithClosestNode const& stop) {
        rankedNodes.push_back(stop.id);
        nodeToRank.insert({stop.id, rankedNodes.size() - 1});
    });

    // NOTE : here, rankedNodes must contain the stops in the same order that the input rankedNodes.

    // then we can rank the other nodes in the graph :
    for (auto edge: edgesWithStops) {
        if (nodeToRank.find(edge.node_from.id) == nodeToRank.end()) {
            rankedNodes.push_back(edge.node_from.id);
            nodeToRank.insert({edge.node_from.id, rankedNodes.size() - 1});
        }
        if (nodeToRank.find(edge.node_to.id) == nodeToRank.end()) {
            rankedNodes.push_back(edge.node_to.id);
            nodeToRank.insert({edge.node_to.id, rankedNodes.size() - 1});
        }
    }
    return {rankedNodes, nodeToRank};
}

std::vector<my::Edge> _makeEdgesBidirectional(std::vector<my::Edge> const& edges) {
    // For each edge, adds its opposite edge (this doubles the number of edges in the edgelist)
    // note : the function is cleaner without side-effects (taking a const ref to edges, returning a copy)
    // but as this is slower, if performance issues arise, we can mutate edges instead
    std::vector<my::Edge> opposites(edges);
    for (auto edge: edges) {
        Polyline geom = edge.geometry;
        std::reverse(geom.begin(), geom.end());
        opposites.emplace_back(
            edge.node_to.id,
            edge.node_from.id,
            std::move(geom),
            edge.length_m,
            edge.weight
        );
    }
    return opposites;
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
    polygon{get_polygon(polygonFile)} {
    edges = osm_to_graph(osmFile, polygon, walkspeedKmPerHour);

    // extend graph with stop-edges :
    std::tie(edgesWithStops, stopsWithClosestNode) = extend_graph(stops, edges, walkspeedKmPerHour);

    tie(rankedNodes, nodeToRank) = _rankNodes(edgesWithStops, stopsWithClosestNode);
    std::cout << "nb ranked nodes (1) = " << rankedNodes.size() << std::endl;
    std::cout << "nb ranked nodes (2) = " << nodeToRank.size() << std::endl;
    // FIXME = assert the equality here

    bidirectionalEdges = _makeEdgesBidirectional(edgesWithStops);
    nodeToOutEdges = _mapNodesToOutEdges(bidirectionalEdges, nodeToRank);
    std::cout << "The association map has " << nodeToOutEdges.size() << " items" << std::endl;
}


}

