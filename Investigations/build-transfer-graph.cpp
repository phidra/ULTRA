#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"
#include "../DataStructures/Geometry/Point.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Parsing/stopfile.h"
#include "../Custom/Graph/extending_with_stops.h"
#include "../Custom/Graph/graph.h"
#include "../Custom/Dumping/geojson.h"

inline void usage(const std::string binary_name) noexcept {
    std::cout << "Usage:  " << binary_name << "  <osmfile>  <polygonfile>  <stopfile>  <output-dir>" << std::endl;
    exit(0);
}

int main(int argc, char** argv) {
    if (argc < 5)
        usage(argv[0]);

    const std::string osmfile = argv[1];
    const std::string polygonfile = argv[2];
    auto stopfile = argv[3];
    std::string output_dir = argv[4];
    if (output_dir.back() != '/') {
        output_dir.push_back('/');
    }
    std::ifstream stopfile_stream{stopfile};
    if (!stopfile_stream.good()) {
        std::cerr << "ERROR: unable to read stopfile : '" << stopfile << "'\n";
        std::cerr << "\n";
        usage(argv[0]);
        std::exit(2);
    }

    std::cout << "OSMFILE          = " << osmfile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonfile << std::endl;
    std::cout << "STOPFILE         = " << stopfile << std::endl;
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

    // parse stopfile early, in order to fail early if needed :
    std::vector<my::Stop> stops = my::parse_stopfile(stopfile, stopfile_stream);

    std::cout << "Building edges from OSM graph..." << std::endl;
    float walkspeed_km_per_h = 4.7;
    auto edges = my::osm_to_graph(osmfile, polygon, walkspeed_km_per_h);
    std::cout << "Number of edges in graph : " << edges.size() << std::endl;

    TransferGraph ultragraph;
    ultragraph.addVertices(100);

    Geometry::Point antony_latlon{Construct::LatLongTag{}, 48.761138, 2.306042};
    Geometry::Point paris_latlon{Construct::LatLongTag{}, 48.854661, 2.340363};
    Vertex antony = ultragraph.addVertex(antony_latlon);
    Vertex paris = ultragraph.addVertex(paris_latlon);
    std::cout << "Antony coords is : " << antony_latlon << std::endl;
    std::cout << "Paris coords is : " << paris_latlon << std::endl;

    // NOTE : with addEdge, we can only add edges from the LAST vertex of the graph :
    // auto edge = ultragraph.addEdge(antony, paris);  // this won't work as "antony" is not the last vertex
    auto edge = ultragraph.addEdge(paris, antony);  // this works bc paris is the last vertex
    edge.set(TravelTime, 42);
    std::cout << "TravelTime of edge = " << ultragraph.get(TravelTime, edge) << std::endl;
    std::cout << "Number of vertices in ultragraph : " << ultragraph.numVertices() << std::endl;
    std::cout << "Number of edges    in ultragraph : " << ultragraph.numEdges() << std::endl;


    std::ofstream original_graph_stream(output_dir + "original_graph.geojson");
    my::dump_geojson_graph(original_graph_stream, edges);

    std::ofstream polygon_stream(output_dir + "polygon.geojson");
    my::dump_geojson_line(polygon_stream, polygon.outer());

    // extend graph with stop-edges :
    auto [edges_with_stops,stops_with_closest_node] = my::extend_graph(stops, edges, walkspeed_km_per_h);
    std::cout << "nb edges including stops = " << edges_with_stops.size() << std::endl;
    std::cout << "nb stops = " << stops_with_closest_node.size() << std::endl;

    std::ofstream extended_graph_stream(output_dir + "graph_with_stops.geojson");
    dump_geojson_graph(extended_graph_stream, edges_with_stops);

    std::ofstream stops_stream(output_dir + "stops.geojson");
    dump_geojson_stops(stops_stream, stops_with_closest_node);

    return 0;
}
