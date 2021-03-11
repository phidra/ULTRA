#include <iostream>

#include "geojson.h"
#include "polygon.h"
#include "gtfs_processing.h"

using namespace std;


int main(int argc, char** argv) {

    cout << "Test" << endl;

    // Common :
    my::BgPolygon poly = my::create_polygon({{1,1}, {2,2}, {3,3}, {1,1}});
    cout << "Empty ? " << my::is_empty(poly) << endl;

    vector<my::Edge> edges;
    my::dump_geojson_graph(cout, edges);
    cout << endl;

    // Preprocess :
    my::preprocess::RouteLabel label = "15+16+17+42";
    auto splitted = my::preprocess::routeToStops(label);
    cout << "Route = " << label << endl;
    for (auto stop: splitted) {
        cout << "\t" << stop << endl;
    }
    cout << endl;

    return 0;
}
