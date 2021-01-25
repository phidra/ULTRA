#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../Custom/gtfs.h"

using namespace std;

inline void usage() noexcept {
    cout << "Usage: converter <GTFS folder>" << endl;
    exit(1);
}

vector<RAPTOR::Stop> convert_stopData(ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::Stop> stopData;
    for (auto const & [ stop_id, stop_ptr ] : feed.getStops()) {
        if (stop_ptr == 0) {
            cout << "ERROR : stop_ptr is 0 for id : " << stop_id << endl;
            exit(1);
        }
        auto const& stop = *stop_ptr;
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData.emplace_back(stop.getName(), location);
    }
    return stopData;
}

vector<RAPTOR::Route> convert_routeData(map<my::StopSetId, set<my::TripId> > const& stopsetToTrips) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(stopsetToTrips.size());
    transform(stopsetToTrips.begin(), stopsetToTrips.end(), back_inserter(routeData),
              [](auto& stopset_to_trip) { return RAPTOR::Route(stopset_to_trip.first); });
    return routeData;
}

pair<vector<my::StopId>, vector<size_t> > convert_stopIdsRelated(vector<RAPTOR::Route> const& routeData) {
    vector<my::StopId> stopIds;
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);

    size_t current_route_first_stop = 0;
    int route_index = 0;
    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a "route" name is its stopset
        vector<my::StopId> this_route_stops = my::stopset_id_to_stops(routeName);
        int this_route_nb_stops = this_route_stops.size();

        firstStopIdOfRoute[route_index++] = current_route_first_stop;
        move(this_route_stops.begin(), this_route_stops.end(), back_inserter(stopIds));
        current_route_first_stop += this_route_nb_stops;
    }

    // À ce stade :
    //      route_index = nombre de routes
    //      current_route_first_stop = nombre de stops
    // On sette l'index "past-the-end" de stopIds :
    firstStopIdOfRoute[route_index] = current_route_first_stop;

    return {stopIds, firstStopIdOfRoute};
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

    // stopData :
    vector<RAPTOR::Stop> stopData = convert_stopData(feed);
    cout << "À ce stade, stopData contient : " << stopData.size() << " items." << endl;
    int counter = 0;
    for (auto& stop : stopData) {
        if (counter++ <= 8) {
            cout << "STOP = " << stop << endl;
        }
    }

    // routeData :
    auto[stopsetToTrips, routesToStopsets] = my::partition_trips_in_stopsets(feed);
    vector<RAPTOR::Route> routeData = convert_routeData(stopsetToTrips);
    cout << "À ce stade, routeData contient : " << routeData.size() << " items." << endl;
    counter = 0;
    for (auto& route : routeData) {
        if (counter++ <= 8) {
            cout << "ROUTE = " << route << endl;
        }
    }

    // stopIds + firstStopIdOfRoute :
    auto[stopIds, firstStopIdOfRoute] = convert_stopIdsRelated(routeData);
    cout << "À ce stade, stopIds contient : " << stopIds.size() << " items." << endl;
    cout << "À ce stade, firstStopIdOfRoute contient : " << firstStopIdOfRoute.size() << " items." << endl;
    counter = 0;
    for (auto idx : firstStopIdOfRoute) {
        if (counter++ <= 8) {
            cout << "First stop id of this = " << idx << endl;
        }
    }
    cout << "Le dernier élément de firstStopIdOfRoute est " << firstStopIdOfRoute.back() << endl;

    return 0;
}
