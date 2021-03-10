#include <iostream>
#include <map>

#include "transfer_graph.h"

#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/Geometry/Point.h"
#include "../../DataStructures/Graph/Classes/StaticGraph.h"
#include "Parsing/polygonfile.h"
#include "Parsing/gtfs_stops.h"
#include "Graph/extending_with_stops.h"
#include "Graph/graph.h"
#include "../Common/geojson.h"
#include "../Common/autodeletefile.h"


namespace my::preprocess {

std::pair<std::vector<my::NodeId>, std::unordered_map<my::NodeId, size_t>>
_rankNodes(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stops) {
    std::vector<my::NodeId> rankedNodes;
    std::unordered_map<my::NodeId, size_t> nodeToRank;

    // ULTRA needs that stops are the first nodes of the graph -> stops must be ranked first :
    std::for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &rankedNodes](my::StopWithClosestNode const& stop) {
        rankedNodes.push_back(stop.id);
        nodeToRank.insert({stop.id, rankedNodes.size() - 1});
    });

    // NOTE : here, rankedNodes must contain the stops in the same order that the input rankedNodes.

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
    // to build the ULTRA transfer graph, we need that all the edges are in so-called "proper" order
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

std::pair<std::map<size_t, std::vector<size_t>>, std::vector<my::Edge>> _mapNodesToOutEdges(std::vector<my::Edge> const& edges, std::unordered_map<my::NodeId, size_t> const& nodeToRank) {
    // this functions helps to retrieve the out-edges of a node, given its rank
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;
    for (int i = 0; i < nodeToRank.size(); ++i) {
        nodeToOutEdges[i] = {};
    }

    // FIXME : on prépare les edges avec une copie :
    std::vector<my::Edge> edgesWithReversed(edges);
    for (auto edge: edges) {
        Polyline geom = edge.geometry;
        std::reverse(geom.begin(), geom.end());
        edgesWithReversed.emplace_back(
            edge.node_to.id,
            edge.node_from.id,
            std::move(geom),
            edge.length_m,
            edge.weight
        );
    }

    size_t edge_index = 0;
    for (auto edge: edgesWithReversed) {
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        size_t node_to_rank = nodeToRank.at(edge.node_to.id);
        /* FIXME> EDIT : this is not necessary anymore... */
        /* if (node_from_rank < node_to_rank) { */
        /*     std::cout << "ERROR : nodes are not properly ordered !" << std::endl; */
        /*     std::exit(4); */
        /* } */

        /* // as the edges are bidirectionnal, each edge has to be added as an out-edge of node_from AND as an out-edge of node_to : */
        nodeToOutEdges[node_from_rank].push_back(edge_index);
        /* nodeToOutEdges[node_to_rank].push_back(edge_index); */
        ++edge_index;
    }
    return {nodeToOutEdges, edgesWithReversed};
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
            transferGraph.setVertexAttributes(currentVertex, currentVertexLatlon);  // FIXME: could probably be done only once (EDIT : but not for stops)
            transferGraph.setVertexAttributes(Vertex{target_vertex_rank}, targetVertexLatlon);  // FIXME: could probably be done only once (EDIT : but not for stops)

            // FIXME: weight in graph is an int -> travel-time is converted in deciseconds :
            auto used_weight = 10 * edge.weight;
            addedEdge.set(TravelTime, static_cast<int>(used_weight));
        }

        ++vertex_rank;
    }

    return transferGraph;
}


std::tuple<TransferGraph::VertexAttributes, TransferGraph::EdgeAttributes, std::vector<::Edge> >
buildTransferGraphStructures(
    std::vector<my::Edge> const& edgesWithStops,
    std::vector<my::StopWithClosestNode> const& stopsWithClosestNode,
    std::vector<my::NodeId> const& rankedNodes,
    std::unordered_map<my::NodeId, size_t> const& nodeToRank,
    std::map<size_t, std::vector<size_t>> const& nodeToOutEdges
) {
    // FIXME : grosse passe de mise au propre nécessaire
    // ICI, je crée les nodes et leur coordonnées (pas très pratique, je n'ai les coordonnées que dans les edges...) :
    std::cout << std::endl;
    std::cout << "Combien d'items dans les stops : " << stopsWithClosestNode.size() << std::endl;
    std::cout << "Combien d'items dans les rankedNodes : " << rankedNodes.size() << std::endl;
    std::cout << std::endl;

    TransferGraph::VertexAttributes vertexAttrs(rankedNodes.size());
    for (auto edge: edgesWithStops) {
        Geometry::Point node_from_coords{Construct::LatLongTag{}, edge.node_from.lat(), edge.node_from.lon()};
        size_t node_from_rank = nodeToRank.at(edge.node_from.id);
        vertexAttrs.set(Coordinates, ::Vertex{node_from_rank}, node_from_coords);

        Geometry::Point node_to_coords{Construct::LatLongTag{}, edge.node_to.lat(), edge.node_to.lon()};
        size_t node_to_rank = nodeToRank.at(edge.node_to.id);
        vertexAttrs.set(Coordinates, ::Vertex{node_to_rank}, node_to_coords);
    }

    // ET LA, je créé les vertex et leur attributs (ToVertex + TravelTime) :
    TransferGraph::EdgeAttributes edgeAttrs(2*edgesWithStops.size());  // each edge will be added twice, in each direction
    std::vector<::Edge> beginOut;
    // NOTE : ici, il faut plutôt itérer sur les nodes dans l'ordre (si pas encore fait)
    int edge_counter{0};
    for (auto [vertex, outEdges]: nodeToOutEdges) {
        beginOut.push_back(::Edge{edge_counter});
        for (auto outEdgeIdx: outEdges) {
            auto edge = edgesWithStops.at(outEdgeIdx);

            auto node_to_rank = nodeToRank.at(edge.node_to.id);
            edgeAttrs.set(ToVertex, ::Edge{edge_counter}, Vertex{node_to_rank});

            auto travel_time = edge.weight;
            edgeAttrs.set(TravelTime, ::Edge{edge_counter}, 10*travel_time + 1);
            ++edge_counter;
        }
    }
    beginOut.push_back(::Edge{edge_counter});

    std::cout << "À ce stade, nombre d'items : " << std::endl;
    std::cout << "\t beginOut    = " << beginOut.size() << std::endl;
    std::cout << "\t vertexAttrs = " << vertexAttrs.size() << std::endl;
    std::cout << "\t edgeAttrs   = " << edgeAttrs.size() << std::endl;
    std::cout << "\t nb_nodes    = " << rankedNodes.size() << std::endl;
    std::cout << "\t nb_edges    = " << edgesWithStops.size() << std::endl;
    std::cout << "\t 2*nb_edges  = " << 2*edgesWithStops.size() << std::endl;
    return {vertexAttrs, edgeAttrs, beginOut};
}

TransferGraph buildTransferGraph(std::vector<my::Edge> const& edgesWithStops, std::vector<my::StopWithClosestNode> const& stopsWithClosestNode) {
    auto [rankedNodes, nodeToRank_] = _rankNodes(edgesWithStops, stopsWithClosestNode);
    // due to a bug in clang, nodeToRank is not capturable in the lambda, unless we alias it :
    auto& nodeToRank = nodeToRank_;
    std::cout << "nb ranked nodes (1) = " << rankedNodes.size() << std::endl;
    std::cout << "nb ranked nodes (2) = " << nodeToRank.size() << std::endl;

    std::vector<my::Edge> properOrderEdges = _revertEdgesIfNecessary(edgesWithStops, nodeToRank);
    std::cout << "How many edges in properOrderEdges ? " << properOrderEdges.size() << std::endl;

    auto [nodeToOutEdges, edgesWithReversed] = _mapNodesToOutEdges(properOrderEdges, nodeToRank);
    std::cout << "The association map has " << nodeToOutEdges.size() << " items" << std::endl;

    // building structures of transferGraph :
    auto [vertexAttrs, edgeAttrs, beginOut] = buildTransferGraphStructures(
        edgesWithReversed,
        stopsWithClosestNode,
        rankedNodes,
        nodeToRank,
        nodeToOutEdges
    );

    // serialization :
    // FIXME : set a proper name :
    std::string fileName = "/tmp/dasuperpouet";
    const std::string separator = ".";
    IO::serialize(fileName + separator + "beginOut", beginOut);
    vertexAttrs.serialize(fileName, separator);
    edgeAttrs.serialize(fileName, separator);

    // building transfergraph by deserializing :
    // FIXME : modify transferGraph to build directly ?
    TransferGraph transferGraph;
    transferGraph.readBinary(fileName);
    Graph::writeStatisticsFile(transferGraph, fileName, separator);

    // FIXME : remove this properly :
    /* auto transferGraph = _computeTransferGraph(rankedNodes, nodeToOutEdges, properOrderEdges, nodeToRank); */
    return transferGraph;
}


my::preprocess::UltraTransferData::UltraTransferData(
    std::filesystem::path osmFile,
    std::filesystem::path polygonFile,
    std::vector<RAPTOR::Stop> const& raptor_stops,
    float walkspeedKmPerHour_) :
    walkspeedKmPerHour{walkspeedKmPerHour_},
    polygon{get_polygon(polygonFile)} {

    std::vector<my::Stop> stops;
    for (int stopRank = 0; stopRank < raptor_stops.size(); ++stopRank) {
        auto const& stop = raptor_stops[stopRank];
        stops.emplace_back(
            stop.coordinates.longitude,
            stop.coordinates.latitude,
            std::to_string(stopRank),
            stop.name
        );
    }

    edges = my::preprocess::osm_to_graph(osmFile, polygon, walkspeedKmPerHour);

    // extend graph with stop-edges :
    std::tie(edgesWithStops,stopsWithClosestNode) = my::preprocess::extend_graph(stops, edges, walkspeedKmPerHour);
    transferGraph = my::preprocess::buildTransferGraph(edgesWithStops, stopsWithClosestNode);
}


void my::preprocess::UltraTransferData::dumpIntermediary(std::string const& outputDir) const {
    std::ofstream originalGraphStream(outputDir + "original_graph.geojson");
    my::dump_geojson_graph(originalGraphStream, edges);

    std::ofstream polygonStream(outputDir + "polygon.geojson");
    my::dump_geojson_line(polygonStream, polygon.outer());

    std::ofstream extendedGraphStream(outputDir + "graph_with_stops.geojson");
    my::dump_geojson_graph(extendedGraphStream, edgesWithStops);

    std::ofstream stopsStream(outputDir + "stops.geojson");
    my::dump_geojson_stops(stopsStream, stopsWithClosestNode);
}

bool my::preprocess::UltraTransferData::areApproxEqual(TransferGraph const& left, TransferGraph const& right) {
    // we only check the stats :
    std::ostringstream statsLeft_stream;
    left.printAnalysis(statsLeft_stream);
    const std::string statsLeft = statsLeft_stream.str();

    std::ostringstream statsRight_stream;
    right.printAnalysis(statsRight_stream);
    const std::string statsRight = statsRight_stream.str();

    return (statsLeft == statsRight);
}

bool my::preprocess::UltraTransferData::checkSerializationIdempotence() const {
    my::AutoDeleteTempFile tmpfile;

    transferGraph.writeBinary(tmpfile.file);

    // unserializing, to see if serialization+serialization is idempotent :
    TransferGraph freshTransferGraph;
    freshTransferGraph.readBinary(tmpfile.file);

    return my::preprocess::UltraTransferData::areApproxEqual(transferGraph, freshTransferGraph);
}

}
