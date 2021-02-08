#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"
#include "../DataStructures/Geometry/Point.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Graph/graph.h"
#include "../Custom/Dumping/geojson.h"

inline void usage(const std::string binary_name) noexcept {
    std::cout << "Usage:  " << binary_name << "  <osmfile>  <polygonfile>" << std::endl;
    exit(0);
}

int main(int argc, char** argv) {
    if (argc < 3)
        usage(argv[0]);

    const std::string osmfile = argv[1];
    const std::string polygonfile = argv[2];

    std::cout << "osmfile          = " << osmfile << std::endl;
    std::cout << "polygonfile      = " << polygonfile << std::endl;
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


    //ofstream geojson_graph(output_dir + "graph.geojson");
    std::ofstream geojson_graph("/tmp/graph.geojson");
    my::dump_geojson_graph(geojson_graph, edges);

    std::ofstream out_polygon("/tmp/polygon.geojson");
    my::dump_geojson_line(out_polygon, polygon.outer());

    return 0;
}
