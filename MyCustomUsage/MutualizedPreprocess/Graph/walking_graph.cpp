#include <iostream>
#include <fstream>
#include <map>

#include "Graph/walking_graph.h"

#include "Graph/polygonfile.h"
#include "Graph/extending_with_stops.h"
#include "Graph/graph.h"
#include "Graph/geojson.h"

using namespace std;

namespace my::preprocess {

unordered_map<my::NodeId, size_t> _rankNodes(vector<my::Edge> const& edgesWithStops, vector<my::Stop> const& stops) {
    unordered_map<my::NodeId, size_t> nodeToRank;

    // some algorithms (ULTRA) need that stops are the first nodes of the graph -> stops must be ranked first :
    size_t rank = 0;
    for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rank](my::Stop const& stop) {
        nodeToRank.insert({stop.id, rank++});
    });

    auto rankThatNode = [&nodeToRank, &rank](auto const& node_id) {
        if (nodeToRank.find(node_id) == nodeToRank.end()) {
            nodeToRank.insert({node_id, rank++});
        }
    };

    // then we can rank the other nodes in the graph :
    for (auto edge : edgesWithStops) {
        rankThatNode(edge.node_from.id);
        rankThatNode(edge.node_to.id);
    }

    return nodeToRank;
}

vector<my::Edge> _makeEdgesBidirectional(vector<my::Edge> const& edges) {
    // For each edge, adds its opposite edge (this doubles the number of edges in the edgelist)
    // note : the function is cleaner without side-effects (taking a const ref to edges, returning a copy)
    // but as this is slower, if performance issues arise, we can mutate edges instead
    vector<my::Edge> bidirectional(edges);
    for (auto edge : edges) {
        Polyline geom = edge.geometry;
        reverse(geom.begin(), geom.end());
        bidirectional.emplace_back(edge.node_to.id, edge.node_from.id, move(geom), edge.length_m, edge.weight);
    }
    assert(bidirectional.size() == 2 * edges.size());
    return bidirectional;
}

map<size_t, vector<size_t>> _mapNodesToOutEdges(vector<my::Edge> const& edges,
                                                unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // this functions build a map that helps to retrieve the out-edges of a node (given its rank)
    map<size_t, vector<size_t>> nodeToOutEdges;
    for (size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
        auto const& edge = edges[edge_index];
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        nodeToOutEdges[node_from_rank].push_back(edge_index);
    }
    return nodeToOutEdges;
}

WalkingGraph::WalkingGraph(filesystem::path osmFile,
                           filesystem::path polygonFile,
                           vector<my::Stop> const& stops,
                           float walkspeedKmPerHour_)
    : walkspeedKmPerHour{walkspeedKmPerHour_},
      polygon{get_polygon(polygonFile)},
      edgesOsm{osm_to_graph(osmFile, polygon, walkspeedKmPerHour)} {
    // extend graph with stop-edges :
    tie(edgesWithStops, stopsWithClosestNode) = extend_graph(stops, edgesOsm, walkspeedKmPerHour);

    nodeToRank = _rankNodes(edgesWithStops, stops);
    cout << "nb ranked nodes = " << nodeToRank.size() << endl;

    edgesWithStopsBidirectional = _makeEdgesBidirectional(edgesWithStops);
    nodeToOutEdges = _mapNodesToOutEdges(edgesWithStopsBidirectional, nodeToRank);
    cout << "The association map has " << nodeToOutEdges.size() << " items" << endl;
}

void WalkingGraph::dumpIntermediary(string const& outputDir) const {
    ofstream originalGraphStream(outputDir + "original_graph.geojson");
    my::dump_geojson_graph(originalGraphStream, edgesOsm);

    ofstream extendedGraphStream(outputDir + "graph_with_stops.geojson");
    my::dump_geojson_graph(extendedGraphStream, edgesWithStops);

    ofstream stopsStream(outputDir + "stops.geojson");
    my::dump_geojson_stops(stopsStream, stopsWithClosestNode);

    ofstream polygonStream(outputDir + "polygon.geojson");
    my::dump_geojson_line(polygonStream, polygon.outer());
}

}  // namespace my::preprocess
