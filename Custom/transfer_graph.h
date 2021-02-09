#pragma once

#include <iostream>
#include <unordered_set>
#include <map>

#include "../DataStructures/Geometry/Point.h"
#include "../DataStructures/Graph/Graph.h"

#include "Common/graphtypes.h"

// note : ULTRA code is not safe to use in multiple translation units, thus all the code is exposed in this header...

namespace my {

std::pair<std::vector<my::NodeId>, std::unordered_map<my::NodeId, size_t>>
rankNodes(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stops) {
    // ranking vertices :
    std::vector<my::NodeId> rankedNodes;
    std::unordered_map<my::NodeId, size_t> nodeToRank;

    // stops must be ranked first :
    std::for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rankedNodes](my::StopWithClosestNode const& stop) {
        rankedNodes.push_back(stop.id);
        nodeToRank.insert({stop.id, rankedNodes.size() - 1});
    });

    // then we can rank the other nodes used in the graph :
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

std::vector<my::Edge> revertEdgesIfNecessary(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // in the output, all the edges are in "proper" order : they go from higher rank node to lower rank node.
    // (this assumes that edges are bidirectional)
    std::vector<my::Edge> properOrderEdges;
    properOrderEdges.reserve(edges.size());
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(properOrderEdges), [&nodeToRank](auto& edge) {
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        size_t node_to_rank = nodeToRank.at(edge.node_to.id);
        // we want the higher rank to be the first node of the edge :
        if (node_from_rank > node_to_rank) {
            return edge;
        }
        return my::Edge::build_reverted(edge);
    });
    return properOrderEdges;
}

std::map<size_t, std::vector<size_t>> mapNodesToOutEdges(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;
    for (int i = 0; i < nodeToRank.size(); ++i) {
        nodeToOutEdges[i] = {};
    }
    size_t edge_index = 0;
    for (auto edge: edges) {
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        size_t node_to_rank = nodeToRank.at(edge.node_to.id);
        if (node_from_rank < node_to_rank) {
            std::cout << "ERROR : nodes are not properly ordered !" << std::endl;
        }
        nodeToOutEdges[node_from_rank].push_back(edge_index);
        ++edge_index;
    }
    return nodeToOutEdges;
}

TransferGraph computeTransferGraph(
    std::vector<my::NodeId> rankedNodes,
    std::map<size_t, std::vector<size_t>> nodeToOutEdges,
    std::vector<my::Edge> const& edges,
    std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    TransferGraph transferGraph;
    size_t vertex_rank = 0;

    for (auto node_id: rankedNodes) {

        // 1. ajouter le vertex au graphe :
        Vertex currentVertex = transferGraph.addVertex();
        if (static_cast<size_t>(currentVertex) != vertex_rank) {
            std::cout << "ERROR : le rank devrait être l'id du vertex." << std::endl;
            std::exit(4);
        }

        // 2. ajouter chaque out-edges (vu les étapes précédentes, ils ne vont que vers des nodes déjà existants) :
        auto& currentVertexOutEdges = nodeToOutEdges[vertex_rank];
        for (auto& outEdgeIndex: currentVertexOutEdges) {
            auto edge = edges[outEdgeIndex];
            if (nodeToRank.at(edge.node_from.id) != vertex_rank) {
                std::cout << "ERROR : le rank devrait être l'id du vertex." << std::endl;
                std::exit(5);
            }
            auto target_vertex_rank = nodeToRank.at(edge.node_to.id);
            auto addedEdge = transferGraph.addEdge(currentVertex, Vertex{target_vertex_rank});
            Geometry::Point currentVertexLatlon{Construct::LatLongTag{}, edge.node_from.lat(), edge.node_from.lon()};
            Geometry::Point targetVertexLatlon{Construct::LatLongTag{}, edge.node_to.location.lat(), edge.node_to.location.lon()};
            transferGraph.setVertexAttributes(currentVertex, currentVertexLatlon);  // fixme: could be done only once
            transferGraph.setVertexAttributes(currentVertex, targetVertexLatlon);   // fixme: could be done only once

            // weight in graph is an int -> travel-time is converted in deciseconds :
            auto used_weight = 10 * edge.weight;
            addedEdge.set(TravelTime, static_cast<int>(used_weight));
        }

        ++vertex_rank;
    }

    return transferGraph;
}

TransferGraph buildTransferGraph(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stopsWithClosestNode) {
    auto [rankedNodes, nodeToRank_] = rankNodes(edgesWithStops, stopsWithClosestNode);
    // due to a bug in clang, nodeToRank is not captuable in the lambda, unless we alias it :
    auto& nodeToRank = nodeToRank_;
    std::cout << "nb ranked nodes (1) = " << rankedNodes.size() << std::endl;
    std::cout << "nb ranked nodes (2) = " << nodeToRank.size() << std::endl;


    std::vector<my::Edge> properOrderEdges = revertEdgesIfNecessary(edgesWithStops, nodeToRank);
    std::cout << "How many edges in properOrderEdges ? " << properOrderEdges.size() << std::endl;

    // associate each vertex with its out-edges (so edges towards nodes of smaller rank) :
    std::map<size_t, std::vector<size_t>> nodeToOutEdges = mapNodesToOutEdges(properOrderEdges, nodeToRank);
    std::cout << "The association map has " << nodeToOutEdges.size() << " items" << std::endl;

    // building transfer graph :
    auto transferGraph = computeTransferGraph(rankedNodes, nodeToOutEdges, properOrderEdges, nodeToRank);
    return transferGraph;
}

}
