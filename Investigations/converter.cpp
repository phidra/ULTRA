#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include "../Custom/prepare_gtfs.h"
#include "../Custom/gtfs_to_binary.h"

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

    cout << "Le feed contient " << feed.getRoutes().size() << " routes" << endl;
    cout << "Le feed contient " << feed.getStops().size() << " stops" << endl;
    cout << "Le feed contient " << feed.getTrips().size() << " trips" << endl;

    // prepare GTFS data :
    auto route_to_trips = my::partition_trips_in_routes(feed);
    auto[ranked_routes, route_to_rank] = my::rank_routes(route_to_trips);

    // from now on, we don't need cppgtfs feed anymore.

    // routeData :
    /* vector<RAPTOR::Route> routeData = convert_routeData(route_to_trips); */
    /* cout << "À ce stade, routeData contient : " << routeData.size() << " items." << endl; */
    /* int route_counter = 0; */
    /* for (auto& route : routeData) { */
    /*     if (route_counter++ <= 8) { */
    /*         cout << "ROUTE = " << route << endl; */
    /*     } */
    /* } */

    /* // stopData : */
    /* auto[ranked_stops, stopidToRank] = my::rank_stops(route_to_trips); */
    /* vector<RAPTOR::Stop> stopData = convert_stopData(ranked_stops, feed); */
    /* cout << "À ce stade, stopData contient : " << stopData.size() << " items." << endl; */
    /* int counter = 0; */
    /* for (auto& stop : stopData) { */
    /*     if (counter++ <= 8) { */
    /*         cout << "STOP = " << stop << endl; */
    /*     } */
    /* } */

    /* // stopIds + firstStopIdOfRoute : */
    /* auto[stopIds, firstStopIdOfRoute] = convert_stopIdsRelated(routeData, stopidToRank); */
    /* cout << "À ce stade, stopIds contient : " << stopIds.size() << " items." << endl; */
    /* cout << "À ce stade, firstStopIdOfRoute contient : " << firstStopIdOfRoute.size() << " items." << endl; */
    /* counter = 0; */
    /* for (auto idx : firstStopIdOfRoute) { */
    /*     if (counter++ <= 8) { */
    /*         cout << "First stop id of this route = " << idx << endl; */
    /*     } */
    /* } */
    /* cout << "Le dernier élément de firstStopIdOfRoute est " << firstStopIdOfRoute.back() << endl; */

    /* // stopEvents + firstStopEventOfRoute : */
    /* auto[stopEvents, firstStopEventOfRoute] = convert_stopEventsRelated(feed, routeData, route_to_trips); */
    /* cout << "À ce stade, stopEvents contient : " << stopEvents.size() << " items." << endl; */
    /* cout << "À ce stade, firstStopEventOfRoute contient : " << firstStopEventOfRoute.size() << " items." << endl; */
    /* counter = 0; */
    /* for (auto idx : firstStopEventOfRoute) { */
    /*     if (counter++ <= 8) { */
    /*         cout << "First stop event of this route = " << idx << endl; */
    /*     } */
    /* } */
    /* cout << "Le dernier élément de firstStopEventOfRoute est " << firstStopEventOfRoute.back() << endl; */

    /* // routeSegments + firstRouteSegmentOfStop */
    /* auto[routeSegments, firstRouteSegmentOfStop] = */
    /*     convert_routeSegmentsRelated(routeData, stopidToRank, route_to_trips); */
    /* cout << "À ce stade, routeSegments contient : " << routeSegments.size() << " items." << endl; */
    /* cout << "À ce stade, firstRouteSegmentOfStop contient : " << firstRouteSegmentOfStop.size() << " items." << endl;
     */
    /* counter = 0; */
    /* for (auto idx : firstRouteSegmentOfStop) { */
    /*     if (counter++ <= 8) { */
    /*         cout << "First route segment of this stop = " << idx << endl; */
    /*     } */
    /* } */
    /* cout << "Le dernier élément de firstRouteSegmentOfStop est " << firstRouteSegmentOfStop.back() << endl; */

    return 0;
}
