#include <iostream>
#include <string>
#include <unordered_set>

#include "../DataStructures/RAPTOR/Data.h"
#include "../DataStructures/Geometry/Point.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Parsing/gtfs_stops.h"
#include "../Custom/Graph/extending_with_stops.h"
#include "../Custom/Graph/graph.h"
#include "../Custom/transfer_graph.h"
#include "../Custom/Dumping/geojson.h"

inline void usage(const std::string binary_name) noexcept {
    std::cout << "Usage:  " << binary_name << "  <osmfile>  <polygonfile>  <gtfs-stopfile>  <output-dir>" << std::endl;
    exit(0);
}


int main(int argc, char** argv) {
    if (argc < 5)
        usage(argv[0]);

    const std::string osmfile = argv[1];
    const std::string polygonfile = argv[2];
    auto gtfs_stopfile = argv[3];
    std::string output_dir = argv[4];
    if (output_dir.back() != '/') {
        output_dir.push_back('/');
    }
    std::ifstream stopfile_stream{gtfs_stopfile};
    if (!stopfile_stream.good()) {
        std::cerr << "ERROR: unable to read gtfs_stopfile : '" << gtfs_stopfile << "'\n";
        std::cerr << "\n";
        usage(argv[0]);
        std::exit(2);
    }

    std::cout << "OSMFILE          = " << osmfile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonfile << std::endl;
    std::cout << "STOPFILE         = " << gtfs_stopfile << std::endl;
    std::cout << "OUTPUT_DIR       = " << output_dir << std::endl;
    std::cout << std::endl;

    my::BgPolygon polygon;
    try {
        polygon = get_polygon(polygonfile);
        std::cout << "Is polygon empty = " << my::is_empty(polygon) << std::endl;
    } catch (std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
        usage(argv[0]);
        exit(2);
    } catch (...) {
        std::cout << "UNKNOWN EXCEPTION" << std::endl;
        usage(argv[0]);
        exit(2);
    }

    // parse gtfs_stopfile early, in order to fail early if needed :
    std::vector<my::Stop> stops = my::parse_gtfs_stops(gtfs_stopfile, stopfile_stream);

    std::cout << "Building edges from OSM graph..." << std::endl;
    float walkspeed_km_per_h = 4.7;
    auto edges = my::osm_to_graph(osmfile, polygon, walkspeed_km_per_h);
    std::cout << "Number of edges in original graph : " << edges.size() << std::endl;

    std::ofstream original_graph_stream(output_dir + "original_graph.geojson");
    my::dump_geojson_graph(original_graph_stream, edges);

    std::ofstream polygon_stream(output_dir + "polygon.geojson");
    my::dump_geojson_line(polygon_stream, polygon.outer());

    // extend graph with stop-edges :
    auto [edgesWithStops,stopsWithClosestNode] = my::extend_graph(stops, edges, walkspeed_km_per_h);
    std::cout << "nb edges (including added stops) = " << edgesWithStops.size() << std::endl;
    std::cout << "nb stops = " << stopsWithClosestNode.size() << std::endl;

    std::ofstream extended_graph_stream(output_dir + "graph_with_stops.geojson");
    dump_geojson_graph(extended_graph_stream, edgesWithStops);

    std::ofstream stops_stream(output_dir + "stops.geojson");
    dump_geojson_stops(stops_stream, stopsWithClosestNode);


    auto transferGraph = my::buildTransferGraph(edgesWithStops, stopsWithClosestNode);
    std::cout << "The transferGraph has these vertices : " << transferGraph.numVertices() << std::endl;
    std::cout << "The transferGraph has these edges    : " << transferGraph.numEdges() << std::endl;

    // serializing :
    const std::string outputFileName = output_dir + "serialized.binary.graph";
    transferGraph.writeBinary(outputFileName);
    std::cout << "TransferGraph dumped in : " << outputFileName << std::endl;

    // unserializing, to see if serialization+serialization is idempotent :
    TransferGraph freshTransferGraph;
    freshTransferGraph.readBinary(outputFileName);

    std::ostringstream stats1_stream;
    transferGraph.printAnalysis(stats1_stream);
    const std::string stats1 = stats1_stream.str();

    std::ostringstream stats2_stream;
    freshTransferGraph.printAnalysis(stats2_stream);
    const std::string stats2 = stats2_stream.str();

    std::cout << "Is serialization+deserialization idempotent ?" << std::endl;
    std::cout << (stats1 == stats2) << std::endl;
    std::cout << stats1 << std::endl;

    return 0;
}
