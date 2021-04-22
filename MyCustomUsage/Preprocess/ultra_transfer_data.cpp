#include <iostream>
#include <map>

#include "ultra_transfer_data.h"

#include "DataStructures/RAPTOR/Data.h"
#include "DataStructures/Geometry/Point.h"
#include "DataStructures/Graph/Classes/StaticGraph.h"
#include "MutualizedPreprocess/Graph/graph.h"
#include "MutualizedPreprocess/Graph/geojson.h"
#include "Preprocess/autodeletefile.h"

namespace my::preprocess {

std::tuple<TransferGraph::VertexAttributes, TransferGraph::EdgeAttributes, std::vector<::Edge>>
buildTransferGraphStructures(WalkingGraph const& walkingGraph) {
    // FIXME : grosse passe de mise au propre nécessaire

    // ICI, je crée les nodes et leur coordonnées (pas très pratique, je n'ai les coordonnées que dans les edges...) :
    TransferGraph::VertexAttributes vertexAttrs(walkingGraph.nodeToRank.size());

    for (auto edge : walkingGraph.edgesWithStopsBidirectional) {
        Geometry::Point node_from_coords{Construct::LatLongTag{}, edge.node_from.lat(), edge.node_from.lon()};
        size_t node_from_rank = walkingGraph.nodeToRank.at(edge.node_from.id);
        vertexAttrs.set(Coordinates, ::Vertex{node_from_rank}, node_from_coords);

        Geometry::Point node_to_coords{Construct::LatLongTag{}, edge.node_to.lat(), edge.node_to.lon()};
        size_t node_to_rank = walkingGraph.nodeToRank.at(edge.node_to.id);
        vertexAttrs.set(Coordinates, ::Vertex{node_to_rank}, node_to_coords);
    }

    // ET LA, je créé les vertex et leur attributs (ToVertex + TravelTime) :
    TransferGraph::EdgeAttributes edgeAttrs(walkingGraph.edgesWithStopsBidirectional.size());
    std::vector<::Edge> beginOut;
    // NOTE : ici, il faut plutôt itérer sur les nodes dans l'ordre (si pas encore fait)
    int edge_counter{0};
    for (auto [vertex, outEdges] : walkingGraph.nodeToOutEdges) {
        beginOut.push_back(::Edge{edge_counter});
        for (auto outEdgeIdx : outEdges) {
            auto edge = walkingGraph.edgesWithStopsBidirectional.at(outEdgeIdx);

            auto node_to_rank = walkingGraph.nodeToRank.at(edge.node_to.id);
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
    std::cout << "\t nb_edges    = " << walkingGraph.edgesWithStopsBidirectional.size() << std::endl;
    return {vertexAttrs, edgeAttrs, beginOut};
}

UltraTransferData::UltraTransferData(std::filesystem::path osmFile,
                                     std::filesystem::path polygonFile,
                                     std::vector<RAPTOR::Stop> const& raptor_stops,
                                     float walkspeedKmPerHour)
    : walkingGraph{[&]() {
          // converting RAPTOR::Stop to unopinionated stops :
          std::vector<my::Stop> stops;
          for (int stopRank = 0; stopRank < raptor_stops.size(); ++stopRank) {
              auto const& stop = raptor_stops[stopRank];
              stops.emplace_back(stop.coordinates.longitude, stop.coordinates.latitude, std::to_string(stopRank),
                                 stop.name);
          }

          WalkingGraph walkingGraph{osmFile, polygonFile, stops, walkspeedKmPerHour};
          return walkingGraph;
      }()} {
    auto [vertexAttrs, edgeAttrs, beginOut] = buildTransferGraphStructures(walkingGraph);

    // serialization :
    // FIXME : set a proper name :
    std::string fileName = "/tmp/dasuperpouet";
    const std::string separator = ".";
    IO::serialize(fileName + separator + "beginOut", beginOut);
    vertexAttrs.serialize(fileName, separator);
    edgeAttrs.serialize(fileName, separator);

    // building transfergraph by deserializing :
    // FIXME : modify transferGraphUltra to build directly ?
    transferGraphUltra.readBinary(fileName);
    Graph::writeStatisticsFile(transferGraphUltra, fileName, separator);
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

    transferGraphUltra.writeBinary(tmpfile.file);

    // unserializing, to see if serialization+serialization is idempotent :
    TransferGraph freshTransferGraph;
    freshTransferGraph.readBinary(tmpfile.file);

    return UltraTransferData::areApproxEqual(transferGraphUltra, freshTransferGraph);
}

}  // namespace my::preprocess
