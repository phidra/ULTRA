#include <iostream>
#include <map>

#include "ultra_transfer_data.h"

#include "DataStructures/RAPTOR/Data.h"
#include "DataStructures/Geometry/Point.h"
#include "DataStructures/Graph/Classes/StaticGraph.h"
#include "Preprocess/Graph/walking_graph.h"
#include "Preprocess/Graph/graph.h"
#include "Common/geojson.h"
#include "Common/autodeletefile.h"



namespace my::preprocess {



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

            auto travel_time = edge.weight;  // FIXME : there is a slight rounding mistake here
            edgeAttrs.set(TravelTime, ::Edge{edge_counter}, travel_time);
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


UltraTransferData::UltraTransferData(
    std::filesystem::path osmFile,
    std::filesystem::path polygonFile,
    std::vector<RAPTOR::Stop> const& raptor_stops,
    float walkspeedKmPerHour) {

    // converting RAPTOR::Stop to unopinionated stops :
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

    WalkingGraph walking_graph{osmFile, polygonFile, stops, walkspeedKmPerHour};

    // building structures of transferGraph :
    auto [vertexAttrs, edgeAttrs, beginOut] = buildTransferGraphStructures(
        walking_graph.bidirectionalEdges,
        walking_graph.stopsWithClosestNode,
        walking_graph.rankedNodes,
        walking_graph.nodeToRank,
        walking_graph.nodeToOutEdges
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
    transferGraph.readBinary(fileName);
    Graph::writeStatisticsFile(transferGraph, fileName, separator);
}


void UltraTransferData::dumpIntermediary(std::string const& outputDir) const {
    /* std::ofstream originalGraphStream(outputDir + "original_graph.geojson"); */
    /* my::dump_geojson_graph(originalGraphStream, edges); */

    /* std::ofstream extendedGraphStream(outputDir + "graph_with_stops.geojson"); */
    /* my::dump_geojson_graph(extendedGraphStream, edgesWithStops); */

    /* std::ofstream stopsStream(outputDir + "stops.geojson"); */
    /* my::dump_geojson_stops(stopsStream, stopsWithClosestNode); */
}

bool UltraTransferData::areApproxEqual(TransferGraph const& left, TransferGraph const& right) {
    // we only check the stats :
    std::ostringstream statsLeft_stream;
    left.printAnalysis(statsLeft_stream);
    const std::string statsLeft = statsLeft_stream.str();

    std::ostringstream statsRight_stream;
    right.printAnalysis(statsRight_stream);
    const std::string statsRight = statsRight_stream.str();

    return (statsLeft == statsRight);
}

bool UltraTransferData::checkSerializationIdempotence() const {
    my::AutoDeleteTempFile tmpfile;

    transferGraph.writeBinary(tmpfile.file);

    // unserializing, to see if serialization+serialization is idempotent :
    TransferGraph freshTransferGraph;
    freshTransferGraph.readBinary(tmpfile.file);

    return UltraTransferData::areApproxEqual(transferGraph, freshTransferGraph);
}

}
