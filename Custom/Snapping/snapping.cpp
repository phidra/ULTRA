#include <boost/geometry.hpp>

#include "snapping.h"

using namespace std;
using BgDegree = boost::geometry::cs::spherical_equatorial<boost::geometry::degree>;
using BgPoint = boost::geometry::model::point<double, 2, BgDegree>;
using RtreeValue = pair<BgPoint, string>;
using RTree = boost::geometry::index::rtree<RtreeValue, boost::geometry::index::linear<8>>;

static auto HAVERSINE = boost::geometry::strategy::distance::haversine<double>(6'371'000);
static RTree rtree;

void build_index(myserver::StopMap stops) {
    for (auto pair : stops) {
        auto id = pair.first;
        auto stop = pair.second;
        rtree.insert(make_pair(BgPoint{stop.lon, stop.lat}, id));
    }
}

tuple<string, double, double, float> get_closest_stop(double lon, double lat) {
    vector<RtreeValue> closest_stops;
    BgPoint point{lon, lat};
    rtree.query(boost::geometry::index::nearest(point, 1), back_inserter(closest_stops));
    auto closest_stop_id = closest_stops.front().second;
    auto closest_stop_loc = closest_stops.front().first;
    auto distance = boost::geometry::distance(closest_stop_loc, BgPoint{lon, lat}, HAVERSINE);
    return make_tuple(closest_stop_id, get<0>(closest_stop_loc), get<1>(closest_stop_loc), distance);
}
