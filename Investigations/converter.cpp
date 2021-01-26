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

vector<RAPTOR::Route> convert_routeData(map<my::StopSetId, set<my::TripId> > const& stopsetToTrips) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(stopsetToTrips.size());
    transform(stopsetToTrips.begin(), stopsetToTrips.end(), back_inserter(routeData),
              [](auto& stopset_to_trip) { return RAPTOR::Route(stopset_to_trip.first); });
    return routeData;
}

vector<RAPTOR::Stop> convert_stopData(vector<my::StopId> const& ranked_stops, ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::Stop> stopData(ranked_stops.size());
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        my::StopId stop_id = ranked_stops[rank];
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

pair<vector<StopId>, vector<size_t> > convert_stopIdsRelated(vector<RAPTOR::Route> const& routeData,
                                                             unordered_map<my::StopId, size_t> const& stopidToRank) {
    vector<StopId> stopIds;
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);

    size_t current_route_first_stop = 0;
    int route_index = 0;
    for (auto& route : routeData) {
        my::StopSetId routeName = route.name;  // a (scientific) route's name is its stopset
        vector<my::StopId> this_route_stops = my::stopset_id_to_stops(routeName);

        firstStopIdOfRoute[route_index++] = current_route_first_stop;
        transform(this_route_stops.cbegin(), this_route_stops.cend(), back_inserter(stopIds),
                  [&stopidToRank](my::StopId const& stopid) { return StopId{stopidToRank.at(stopid)}; });
        current_route_first_stop += this_route_stops.size();
    }

    // À ce stade :
    //      route_index = nombre de routes
    //      current_route_first_stop = nombre de stops
    // On sette l'index "past-the-end" de stopIds :
    firstStopIdOfRoute[route_index] = current_route_first_stop;

    return {stopIds, firstStopIdOfRoute};
}

/* pair<vector<RAPTOR::StopEvent>, vector<size_t> > convert_stopEventsRelated( */
/*     ad::cppgtfs::gtfs::Feed const& feed, */
/*     vector<RAPTOR::Route> const& routeData, */
/*     map<my::StopSetId, set<my::TripId> > const& stopsetToTrips) { */
/*     vector<RAPTOR::StopEvent> stopEvents; */
/*     vector<size_t> firstStopEventOfRoute(routeData.size() + 1); */

/*     size_t current_route_first_stopevent = 0; */
/*     int route_index = 0; */

/*     for (auto& route : routeData) { */
/*         my::StopSetId routeName = route.name;  // a "route" name is its stopset */

/*         int nb_stopevents_in_this_route = 0; */

/*         // chaque route (=stopset) est associé à plusieurs trips : */
/*         auto const& trips = stopsetToTrips.at(routeName); */
/*         for (auto& trip_id : trips) { */
/*             // récupération du trip courant : */
/*             auto& trip = my::get_trip(feed, trip_id); */

/*             // chaque trip a un range de stopEvents */
/*             // un RAPTOR::StopEvent = {departureTime,arrivalTime} */
/*             // un cppgtfs::StopTime = {_at,_dt} */
/*             for (auto const& stoptime : trip.getStopTimes()) { */
/*                 int departure_time = stoptime.getDepartureTime().seconds(); */
/*                 int arrival_time = stoptime.getArrivalTime().seconds(); */
/*                 stopEvents.emplace_back(arrival_time, departure_time); */
/*             } */

/*             int nb_stopevents_in_this_trip = trip.getStopTimes().size(); */
/*             nb_stopevents_in_this_route += nb_stopevents_in_this_trip; */
/*         } */

/*         firstStopEventOfRoute[route_index++] = current_route_first_stopevent; */
/*         current_route_first_stopevent += nb_stopevents_in_this_route; */
/*     } */

/*     // À ce stade : */
/*     //      route_index = nombre de routes */
/*     //      current_route_first_stopevent = nombre de stopevents */
/*     // On sette l'index "past-the-end" de stopIds : */
/*     firstStopEventOfRoute[route_index] = current_route_first_stopevent; */

/*     return {stopEvents, firstStopEventOfRoute}; */
/* } */

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

    /* // stopEvents + firstStopEventOfRoute : */
    /* auto[stopEvents, firstStopEventOfRoute] = convert_stopEventsRelated(feed, routeData, stopsetToTrips); */
    /* cout << "À ce stade, stopEvents contient : " << stopEvents.size() << " items." << endl; */
    /* cout << "À ce stade, firstStopEventOfRoute contient : " << firstStopEventOfRoute.size() << " items." << endl; */
    /* counter = 0; */
    /* for (auto idx : firstStopEventOfRoute) { */
    /*     if (counter++ <= 8) { */
    /*         cout << "First stop event of this = " << idx << endl; */
    /*     } */
    /* } */
    /* cout << "Le dernier élément de firstStopEventOfRoute est " << firstStopEventOfRoute.back() << endl; */

    /* // routeSegments + firstRouteSegmentOfStop */
    /* vector<size_t> firstRouteSegmentOfStop(stopData.size() + 1); */
    /* /1* vector<RouteSegment> routeSegments; *1/ */

    /* vector<vector<pair<my::StopSetId, int>>> routesOfAStop(stopData.size());  // associe un stop (via son index) à la
     * liste de ses routes */

    /* set<int> all_the_stops; */

    /* //RouteSegment(const RouteId routeId = noRouteId, const StopIndex stopIndex = noStopIndex) : */
    /* for (auto& route: routeData) { */
    /*     my::StopSetId routeName = route.name;  // a "route" name is its stopset */

    /*     vector<int> this_route_stops = my::stopset_id_to_stops(routeName); */

    /*     int stop_seq = 0;  // TODO = confirmer que le stopseq dans un routeSegment commence bien à 0 (et pas à 1) */
    /*     for (auto stop_int: this_route_stops) { */
    /*         cout << "stop_int = " << stop_int << endl; */
    /*         /1* if (stop_int >= stopData.size()) { cout << "TOO BIG : " << stop_int << " related to " <<
     * stopData.size() << endl; } *1/ */
    /*         all_the_stops.insert(stop_int); */
    /*         /1* vector<pair<my::StopSetId, int>>& routes_of_this_stop = routesOfAStop[stop_int]; *1/ */
    /*         /1* routes_of_this_stop.emplace_back(routeName, stop_seq++); *1/ */
    /*     } */
    /*     // note : ici, il faudrait s'assurer que chaque stop a été vu au moins une fois */
    /* } */

    /* // à ce stade, routesOfAStop associe à un stop la liste des routes qu'il utilise. */

    /* cout << endl; */
    /* cout << "Combien de stops en tout dans le set ? " << all_the_stops.size() << endl; */
    /* cout << "Et dans stopData ? " << stopData.size() << endl; */

    return 0;
}
