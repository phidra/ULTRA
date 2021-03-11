#include <iostream>

#include "Common/geojson.h"
#include "Common/polygon.h"

using namespace std;


int main(int argc, char** argv) {

    cout << "Test" << endl;
    my::BgPolygon poly = my::create_polygon({{1,1}, {2,2}, {3,3}, {1,1}});
    cout << "Empty ? " << my::is_empty(poly) << endl;

    vector<my::Edge> edges;
    my::dump_geojson_graph(cout, edges);

    return 0;
}
