#include <iostream>
#include <map>

#include "transfer_graph.h"

#include "../DataStructures/RAPTOR/Data.h"
#include "../DataStructures/Geometry/Point.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Parsing/gtfs_stops.h"
#include "../Custom/Graph/extending_with_stops.h"
#include "../Custom/Graph/graph.h"
#include "../Custom/transfer_graph.h"
#include "../Custom/Dumping/geojson.h"


// note : ULTRA code is not safe to use in multiple translation units,
// thus all the code must be exposed in this header...

namespace my {

std::pair<std::vector<my::NodeId>, std::unordered_map<my::NodeId, size_t>>
_rankNodes(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stops) {
    std::vector<my::NodeId> rankedNodes;
    std::unordered_map<my::NodeId, size_t> nodeToRank;

    // ULTRA needs that stops are the first nodes of the graph -> stops must be ranked first :
    std::for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rankedNodes](my::StopWithClosestNode const& stop) {
        rankedNodes.push_back(stop.id);
        nodeToRank.insert({stop.id, rankedNodes.size() - 1});
    });

    // then we can rank the other nodes :
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

std::vector<my::Edge> _revertEdgesIfNecessary(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // to build the ULTRA transfer graph, we need that all the edges are in so-caled "proper" order
    // that is : they must go from higher rank node to lower rank node
    // This is not the case by default in the edges.
    // This functions ensures it : in the output, all the edges are in "proper" order.
    // (this assumes that edges are bidirectional, and that reverted them has no effect)
    std::vector<my::Edge> properOrderEdges;
    properOrderEdges.reserve(edges.size());
    std::transform(edges.cbegin(), edges.cend(), std::back_inserter(properOrderEdges), [&nodeToRank](auto& edge) {
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        size_t node_to_rank = nodeToRank.at(edge.node_to.id);
        // we want the higher rank to be the first node of the edge :
        return node_from_rank > node_to_rank ? edge : my::Edge::build_reverted(edge);
    });
    return properOrderEdges;
}

std::map<size_t, std::vector<size_t>> _mapNodesToOutEdges(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // this functions helps to retrieve the out-edges of a node, given its rank
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
            std::exit(4);
        }
        nodeToOutEdges[node_from_rank].push_back(edge_index);
        ++edge_index;
    }
    return nodeToOutEdges;
}

TransferGraph _computeTransferGraph(
    std::vector<my::NodeId> rankedNodes,
    std::map<size_t, std::vector<size_t>> nodeToOutEdges,
    std::vector<my::Edge> const& edges,
    std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    TransferGraph transferGraph;
    size_t vertex_rank = 0;

    for (auto node_id: rankedNodes) {

        // first, add vertex to graph :
        Vertex currentVertex = transferGraph.addVertex();
        if (static_cast<size_t>(currentVertex) != vertex_rank) {
            std::cout << "ERROR : rank should be the vertex id here" << std::endl;
            std::exit(4);
        }

        // then, add all its out-edges (by construction, they all go towards an already existing vertex) :
        auto& currentVertexOutEdges = nodeToOutEdges[vertex_rank];
        for (auto& outEdgeIndex: currentVertexOutEdges) {
            auto edge = edges[outEdgeIndex];
            if (nodeToRank.at(edge.node_from.id) != vertex_rank) {
                std::cout << "ERROR : rank should be the vertex id here" << std::endl;
                std::exit(5);
            }
            auto target_vertex_rank = nodeToRank.at(edge.node_to.id);
            auto addedEdge = transferGraph.addEdge(currentVertex, Vertex{target_vertex_rank});
            Geometry::Point currentVertexLatlon{Construct::LatLongTag{}, edge.node_from.lat(), edge.node_from.lon()};
            Geometry::Point targetVertexLatlon{Construct::LatLongTag{}, edge.node_to.location.lat(), edge.node_to.location.lon()};
            transferGraph.setVertexAttributes(currentVertex, currentVertexLatlon);  // FIXME: could be done only once
            transferGraph.setVertexAttributes(currentVertex, targetVertexLatlon);   // FIXME: could be done only once

            // FIXME: weight in graph is an int -> travel-time is converted in deciseconds :
            auto used_weight = 10 * edge.weight;
            addedEdge.set(TravelTime, static_cast<int>(used_weight));
        }

        ++vertex_rank;
    }

    return transferGraph;
}

TransferGraph buildTransferGraph(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stopsWithClosestNode) {
    auto [rankedNodes, nodeToRank_] = _rankNodes(edgesWithStops, stopsWithClosestNode);
    // due to a bug in clang, nodeToRank is not capturable in the lambda, unless we alias it :
    auto& nodeToRank = nodeToRank_;
    std::cout << "nb ranked nodes (1) = " << rankedNodes.size() << std::endl;
    std::cout << "nb ranked nodes (2) = " << nodeToRank.size() << std::endl;

    std::vector<my::Edge> properOrderEdges = _revertEdgesIfNecessary(edgesWithStops, nodeToRank);
    std::cout << "How many edges in properOrderEdges ? " << properOrderEdges.size() << std::endl;

    std::map<size_t, std::vector<size_t>> nodeToOutEdges = _mapNodesToOutEdges(properOrderEdges, nodeToRank);
    std::cout << "The association map has " << nodeToOutEdges.size() << " items" << std::endl;

    auto transferGraph = _computeTransferGraph(rankedNodes, nodeToOutEdges, properOrderEdges, nodeToRank);
    return transferGraph;
}


my::UltraTransferData::UltraTransferData(std::filesystem::path osmFile, std::filesystem::path polygonFile, std::filesystem::path gtfsStopFile) {

    my::BgPolygon polygon = get_polygon(polygonFile);  // may raise

    /* try { */
    /*     polygon = get_polygon(polygonFile); */
    /*     std::cout << "Is polygon empty = " << my::is_empty(polygon) << std::endl; */
    /* } catch (std::exception& e) { */
    /*     std::cout << "EXCEPTION: " << e.what() << std::endl; */
    /*     usage(argv[0]); */
    /*     exit(2); */
    /* } catch (...) { */
    /*     std::cout << "UNKNOWN EXCEPTION" << std::endl; */
    /*     usage(argv[0]); */
    /*     exit(2); */
    /* } */


    std::ifstream stopFileStream{gtfsStopFile};
    if (!stopFileStream.good()) {
        std::cerr << "ERROR: unable to read gtfsStopFile : '" << gtfsStopFile << "'\n";
        std::cerr << "\n";
        raise(42);   // FIXME
    }


    // parse gtfsStopFile early, in order to fail early if needed :
    std::vector<my::Stop> stops = my::parse_gtfs_stops(gtfsStopFile.string().c_str(), stopFileStream);

    std::cout << "Building edges from OSM graph..." << std::endl;
    float walkspeed_km_per_h = 4.7;
    auto edges = my::osm_to_graph(osmFile, polygon, walkspeed_km_per_h);
    std::cout << "Number of edges in original graph : " << edges.size() << std::endl;

    /* std::ofstream original_graph_stream(output_dir + "original_graph.geojson"); */
    /* my::dump_geojson_graph(original_graph_stream, edges); */

    /* std::ofstream polygon_stream(output_dir + "polygon.geojson"); */
    /* my::dump_geojson_line(polygon_stream, polygon.outer()); */

    // extend graph with stop-edges :
    auto [edgesWithStops,stopsWithClosestNode] = my::extend_graph(stops, edges, walkspeed_km_per_h);
    std::cout << "nb edges (including added stops) = " << edgesWithStops.size() << std::endl;
    std::cout << "nb stops = " << stopsWithClosestNode.size() << std::endl;

    /* std::ofstream extended_graph_stream(output_dir + "graph_with_stops.geojson"); */
    /* dump_geojson_graph(extended_graph_stream, edgesWithStops); */

    /* std::ofstream stops_stream(output_dir + "stops.geojson"); */
    /* dump_geojson_stops(stops_stream, stopsWithClosestNode); */


    transferGraph = my::buildTransferGraph(edgesWithStops, stopsWithClosestNode);
    std::cout << "The transferGraph has these vertices : " << transferGraph.numVertices() << std::endl;
    std::cout << "The transferGraph has these edges    : " << transferGraph.numEdges() << std::endl;
}

}
