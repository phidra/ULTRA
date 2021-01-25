#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../Custom/gtfs.h"
#include "../Helpers/Types.h"

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

pair<vector<StopId>, vector<size_t> > convert_stopIdsRelated(vector<RAPTOR::Route> const& routeData) {
    vector<StopId> stopIds;
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);

    size_t current_route_first_stop = 0;
    int route_index = 0;
    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a "route" name is its stopset
        vector<int> this_route_stops = my::stopset_id_to_stops(routeName);
        firstStopIdOfRoute[route_index++] = current_route_first_stop;
        transform(this_route_stops.cbegin(), this_route_stops.cend(), back_inserter(stopIds),
                  [](int stop) { return StopId{stop}; });
        current_route_first_stop += this_route_stops.size();
    }

    // À ce stade :
    //      route_index = nombre de routes
    //      current_route_first_stop = nombre de stops
    // On sette l'index "past-the-end" de stopIds :
    firstStopIdOfRoute[route_index] = current_route_first_stop;

    return {stopIds, firstStopIdOfRoute};
}

pair<vector<RAPTOR::StopEvent>, vector<size_t> > convert_stopEventsRelated(
    ad::cppgtfs::gtfs::Feed const& feed,
    vector<RAPTOR::Route> const& routeData,
    map<my::StopSetId, set<my::TripId> > const& stopsetToTrips) {
    vector<RAPTOR::StopEvent> stopEvents;
    vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t current_route_first_stopevent = 0;
    int route_index = 0;

    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a "route" name is its stopset

        int nb_stopevents_in_this_route = 0;

        // chaque route (=stopset) est associé à plusieurs trips :
        auto const& trips = stopsetToTrips.at(routeName);
        for (auto& trip_id : trips) {
            // récupération du trip courant :
            auto& trip = my::get_trip(feed, trip_id);

            // chaque trip a un range de stopEvents
            // un RAPTOR::StopEvent = {departureTime,arrivalTime}
            // un cppgtfs::StopTime = {_at,_dt}
            for (auto const& stoptime : trip.getStopTimes()) {
                int departure_time = stoptime.getDepartureTime().seconds();
                int arrival_time = stoptime.getArrivalTime().seconds();
                stopEvents.emplace_back(arrival_time, departure_time);
            }

            int nb_stopevents_in_this_trip = trip.getStopTimes().size();
            nb_stopevents_in_this_route += nb_stopevents_in_this_trip;
        }

        firstStopEventOfRoute[route_index++] = current_route_first_stopevent;
        current_route_first_stopevent += nb_stopevents_in_this_route;
    }

    // À ce stade :
    //      route_index = nombre de routes
    //      current_route_first_stopevent = nombre de stopevents
    // On sette l'index "past-the-end" de stopIds :
    firstStopEventOfRoute[route_index] = current_route_first_stopevent;

    return {stopEvents, firstStopEventOfRoute};
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

    // stopEvents + firstStopEventOfRoute :
    auto[stopEvents, firstStopEventOfRoute] = convert_stopEventsRelated(feed, routeData, stopsetToTrips);
    cout << "À ce stade, stopEvents contient : " << stopEvents.size() << " items." << endl;
    cout << "À ce stade, firstStopEventOfRoute contient : " << firstStopEventOfRoute.size() << " items." << endl;
    counter = 0;
    for (auto idx : firstStopEventOfRoute) {
        if (counter++ <= 8) {
            cout << "First stop event of this = " << idx << endl;
        }
    }
    cout << "Le dernier élément de firstStopEventOfRoute est " << firstStopEventOfRoute.back() << endl;

    return 0;
}
