#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../DataStructures/RAPTOR/Entities/RouteSegment.h"
#include "../Custom/gtfs.h"
#include "../Helpers/Types.h"

using namespace std;

inline void usage() noexcept {
    cout << "Usage: converter <GTFS folder>" << endl;
    exit(1);
}

vector<RAPTOR::Route> convert_routeData(map<my::StopSetId, set<my::TripId>> const& stopsetToTrips) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(stopsetToTrips.size());
    transform(stopsetToTrips.begin(), stopsetToTrips.end(), back_inserter(routeData),
              [](auto& stopset_to_trip) { return RAPTOR::Route(stopset_to_trip.first); });
    return routeData;
}

vector<RAPTOR::Stop> convert_stopData(vector<my::ParsedStopId> const& ranked_stops,
                                      ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::Stop> stopData(ranked_stops.size());
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        my::ParsedStopId stop_id = ranked_stops[rank];
        auto stop_ptr = feed.getStops().get(stop_id);
        if (stop_ptr == 0) {
            std::ostringstream oss;
            oss << "ERROR : unable to get stop with id '" << stop_id << "' (stop_ptr is 0)";
            throw std::runtime_error(oss.str());
        }

        auto const& stop = *stop_ptr;
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData[rank] = RAPTOR::Stop{stop.getName(), location};
    }
    return stopData;
}

pair<vector<StopId>, vector<size_t>> convert_stopIdsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<my::ParsedStopId, size_t> const& stopidToRank) {
    vector<StopId> stopIds;
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);

    size_t current_route_first_stop = 0;
    int route_index = 0;
    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a (scientific) route's name is its stopset
        vector<my::ParsedStopId> stops_of_this_route = my::stopset_id_to_stops(routeName);

        firstStopIdOfRoute[route_index++] = current_route_first_stop;
        transform(stops_of_this_route.cbegin(), stops_of_this_route.cend(), back_inserter(stopIds),
                  [&stopidToRank](my::ParsedStopId const& stopid) { return StopId{stopidToRank.at(stopid)}; });
        current_route_first_stop += stops_of_this_route.size();
    }

    // À ce stade :
    //      route_index = nombre de routes
    //      current_route_first_stop = nombre de stops
    // On sette l'index "past-the-end" de stopIds :
    firstStopIdOfRoute[route_index] = current_route_first_stop;

    return {stopIds, firstStopIdOfRoute};
}

pair<vector<RAPTOR::StopEvent>, vector<size_t>> convert_stopEventsRelated(
    ad::cppgtfs::gtfs::Feed const& feed,
    vector<RAPTOR::Route> const& routeData,
    map<my::StopSetId, set<my::TripId>> const& stopsetToTrips) {
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

pair<vector<RAPTOR::RouteSegment>, vector<size_t>> convert_routeSegmentsRelated(
    vector<RAPTOR::Route> const& routeData,  // FIXME : on ne devrait pas dépendre de routeData
    unordered_map<my::ParsedStopId, size_t> const& stopidToRank,
    map<my::StopSetId, set<my::TripId>> const& stopsetToTrips) {
    // build d'une structure intermédiaire associant un stop (via son rank) à la liste de ses routes
    // pour un stop donné, elle contient à la fois la route concernée, et l'index du stop dans la route
    vector<vector<pair<my::StopSetId, int>>> routesOfAStop(stopidToRank.size());

    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a (scientific) route's name is its stopset
        vector<my::ParsedStopId> stops_of_this_route = my::stopset_id_to_stops(routeName);

        for (size_t stop_index = 0; stop_index < stops_of_this_route.size(); ++stop_index) {
            my::ParsedStopId const& this_stop_id = stops_of_this_route[stop_index];
            size_t this_stop_rank = stopidToRank.at(this_stop_id);

            vector<pair<my::StopSetId, int>>& routes_of_this_stop = routesOfAStop[this_stop_rank];
            routes_of_this_stop.emplace_back(routeName, stop_index);
        }
    }

    // À ce stade, on connaît pour chaque stop la liste des routes qui l'utilisent

    // TODO = vérifier que chaque route n'apparaît qu'une fois pour un stop donné

    auto[ranked_routes, routeidToRank] = my::rank_routes(stopsetToTrips);

    vector<size_t> firstRouteSegmentOfStop(stopidToRank.size() + 1);
    vector<RAPTOR::RouteSegment> routeSegments;

    size_t current_stop_first_routesegment = 0;

    for (size_t stop_rank = 0; stop_rank < routesOfAStop.size(); ++stop_rank) {
        auto& routes_of_this_stop = routesOfAStop[stop_rank];

        for (auto & [ route_stopset_id, stop_index_in_this_route ] : routes_of_this_stop) {
            auto route_rank = routeidToRank.at(route_stopset_id);
            routeSegments.emplace_back(RouteId{route_rank}, StopIndex{stop_index_in_this_route});
        }

        firstRouteSegmentOfStop[stop_rank] = current_stop_first_routesegment;
        current_stop_first_routesegment += routes_of_this_stop.size();
    }

    // On sette l'index "past-the-end" de routeSegments :
    firstRouteSegmentOfStop[routesOfAStop.size()] = current_stop_first_routesegment;

    return {routeSegments, firstRouteSegmentOfStop};
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

    // routeData :
    auto[stopsetToTrips, routesToStopsets] = my::partition_trips_in_stopsets(feed);
    vector<RAPTOR::Route> routeData = convert_routeData(stopsetToTrips);
    cout << "À ce stade, routeData contient : " << routeData.size() << " items." << endl;
    int route_counter = 0;
    for (auto& route : routeData) {
        if (route_counter++ <= 8) {
            cout << "ROUTE = " << route << endl;
        }
    }

    // stopData :
    auto[ranked_stops, stopidToRank] = my::rank_stops(stopsetToTrips);
    vector<RAPTOR::Stop> stopData = convert_stopData(ranked_stops, feed);
    cout << "À ce stade, stopData contient : " << stopData.size() << " items." << endl;
    int counter = 0;
    for (auto& stop : stopData) {
        if (counter++ <= 8) {
            cout << "STOP = " << stop << endl;
        }
    }

    // stopIds + firstStopIdOfRoute :
    auto[stopIds, firstStopIdOfRoute] = convert_stopIdsRelated(routeData, stopidToRank);
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

    // routeSegments + firstRouteSegmentOfStop
    auto[routeSegments, firstRouteSegmentOfStop] =
        convert_routeSegmentsRelated(routeData, stopidToRank, stopsetToTrips);

    return 0;
}
