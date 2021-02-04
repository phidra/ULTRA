#include <iostream>
#include <string>

#include "../DataStructures/RAPTOR/Data.h"
#include "../Custom/Parsing/polygonfile.h"
#include "../Custom/Graph/graph.h"

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

    return 0;
}
