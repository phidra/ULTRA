#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include "../Custom/prepare_gtfs.h"
#include "../Custom/gtfs_to_ultra_binary.h"

using namespace std;

inline void usage() noexcept {
    cout << "Usage: converter <GTFS folder>" << endl;
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    const string gtfs_folder = argv[1];

    cout << "Parsing GTFS from folder : " << gtfs_folder << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    cout << "This feed contains " << feed.getRoutes().size() << " routes" << endl;
    cout << "This feed contains " << feed.getStops().size() << " stops" << endl;
    cout << "This feed contains " << feed.getTrips().size() << " trips" << endl;

    // prepare GTFS data :
    auto route_to_trips = my::partition_trips_in_routes(feed);
    auto[ranked_routes, route_to_rank] = my::rank_routes(route_to_trips);
    auto[ranked_stops, stop_to_rank] = my::rank_stops(route_to_trips);

    // routeData :
    vector<RAPTOR::Route> routeData = build_routeData(route_to_trips);
    cout << "Here, routeData contains : " << routeData.size() << " items." << endl;
    int route_counter = 0;
    for (auto& route : routeData) {
        if (route_counter++ <= 8) {
            cout << "ROUTE = " << route << endl;
        }
    }

    // stopData :
    vector<RAPTOR::Stop> stopData = build_stopData(ranked_stops, feed);
    cout << "Here, stopData contains : " << stopData.size() << " items." << endl;
    int counter = 0;
    for (auto& stop : stopData) {
        if (counter++ <= 8) {
            cout << "STOP = " << stop << endl;
        }
    }

    // stopIds + firstStopIdOfRoute :
    auto[stopIds, firstStopIdOfRoute] = build_stopIdsRelated(routeData, stop_to_rank);
    cout << "Here, stopIds contains : " << stopIds.size() << " items." << endl;
    cout << "Here, firstStopIdOfRoute contains : " << firstStopIdOfRoute.size() << " items." << endl;
    counter = 0;
    for (auto idx : firstStopIdOfRoute) {
        if (counter++ <= 8) {
            cout << "First stop id of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopIdOfRoute is " << firstStopIdOfRoute.back() << endl;

    // stopEvents + firstStopEventOfRoute :
    auto[stopEvents, firstStopEventOfRoute] = build_stopEventsRelated(routeData, route_to_trips, feed);
    cout << "Here, stopEvents contains : " << stopEvents.size() << " items." << endl;
    cout << "Here, firstStopEventOfRoute contains : " << firstStopEventOfRoute.size() << " items." << endl;
    counter = 0;
    for (auto idx : firstStopEventOfRoute) {
        if (counter++ <= 8) {
            cout << "First stop event of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopEventOfRoute is " << firstStopEventOfRoute.back() << endl;

    // routeSegments + firstRouteSegmentOfStop
    auto[routeSegments, firstRouteSegmentOfStop] =
        convert_routeSegmentsRelated(routeData, stop_to_rank, route_to_rank, route_to_trips);
    cout << "Here, routeSegments contains : " << routeSegments.size() << " items." << endl;
    cout << "Here, firstRouteSegmentOfStop contains : " << firstRouteSegmentOfStop.size() << " items." << endl;
    counter = 0;
    for (auto idx : firstRouteSegmentOfStop) {
        if (counter++ <= 8) {
            cout << "First route segment of this stop = " << idx << endl;
        }
    }
    cout << "Last item of firstRouteSegmentOfStop is " << firstRouteSegmentOfStop.back() << endl;

    return 0;
}
