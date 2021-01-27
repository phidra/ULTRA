#pragma once

#include <iostream>
#include <string>
#include <numeric>
#include <unordered_set>

#include "ad/cppgtfs/Parser.h"

namespace my {

// About the word "route" :
//  - what science papers calls "route" is a particular group of stops
//    in particular, if two trips travel between the same stops, they have the same route.
//  - what GTFS standard calls "route" is just a given structure associated to a trip
//    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
//    even if they don't use exactly the same set of stops
//
// In ULTRA code, the only meaning of the term "Route" is the scientific one.
// If neeeded, the "route" used in GTFS standard is called GtfsRoute.
//
// NOTE : to enforce different name than those of ULTRA, we use xxxID instead of xxxId

// this is the "scientific" route used in ULTRA code :
using RouteID = std::string;
// this is the id of a "route" in the GTFS data (barely used in ULTRA code) :
using GtfsRouteID = std::string;

using TripID = std::string;
using RouteId = std::string;

using ParsedStopId = std::string;

inline RouteID build_route_id(ad::cppgtfs::gtfs::Trip const& trip) {
    if (trip.getStopTimes().size() < 2) {
        std::ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw std::runtime_error(oss.str());
    }

    RouteID route_id{};

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        route_id.append(stop.getId());
        route_id.append("+");
    }

    // remove final '+' :
    return route_id.substr(0, route_id.size() - 1);
}

/* inline std::vector<ParsedStopId> stopset_id_to_stops(RouteID const& stopset) { */
/*     std::vector<ParsedStopId> stops; */
/*     std::string token; */
/*     std::istringstream iss(stopset); */
/*     while (std::getline(iss, token, '+')) { */
/*         stops.push_back(token); */
/*     } */
/*     return stops; */
/* } */

/* inline RouteId route_id_from_trip_id(ad::cppgtfs::gtfs::Feed const& feed, TripID const& trip_id) { */
/*     auto trip_ptr = feed.getTrips().get(trip_id); */
/*     if (trip_ptr == 0) { */
/*         std::ostringstream oss; */
/*         oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)"; */
/*         throw std::runtime_error(oss.str()); */
/*     } */
/*     auto& trip = *(trip_ptr); */
/*     auto real_route = *(trip.getRoute()); */
/*     return real_route.getId(); */
/* } */

inline std::map<RouteID, std::set<TripID>> partition_trips_in_routes(ad::cppgtfs::gtfs::Feed const& feed) {
    // ULTRA uses "scientific" routes, but feed only has GTFS routes.
    // This functions partitions trips amongst their (scientific) route.
    // Two trips will have the same route IF they have excatly the same stops.

    std::map<RouteID, std::set<TripID>> route_to_trips;

    for (auto const & [ trip_id, trip_ptr ] : feed.getTrips()) {
        auto& trip = *(trip_ptr);
        RouteID this_route_id = build_route_id(trip);
        route_to_trips[this_route_id].emplace(trip_id);
    }
    return route_to_trips;
}

inline std::pair<std::vector<RouteID>, std::unordered_map<RouteID, size_t>> rank_routes(
    std::map<RouteID, std::set<TripID>> const& route_to_trips) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of route)
    // (this rank allows routes to be stored in a vector)

    size_t route_rank = 0;
    std::vector<RouteID> ranked_routes;
    std::unordered_map<RouteID, size_t> route_to_rank;

    for (auto ite = route_to_trips.cbegin(); ite != route_to_trips.cend(); ++ite) {
        RouteID const& routeid = ite->first;
        ranked_routes.push_back(routeid);
        route_to_rank.insert({routeid, route_rank++});
    }

    // Here :
    //   - ranked_routes associates a rank to a route
    //   - route_to_rank associates a route to its rank
    return {ranked_routes, route_to_rank};
}

/* inline std::pair<std::vector<ParsedStopId>, std::unordered_map<ParsedStopId, size_t>> rank_stops( */
/*     std::map<RouteID, std::set<TripID>> const& route_to_trips) { */
/*     // à partir des stops partitionnés en stopsets (i.e. en routes au sens scientifique du terme), */
/*     // cette fonction attribue à chaque stop un rank donné. */

/*     std::set<ParsedStopId> stops; */
/*     for (auto & [ stopset_id, _ ] : route_to_trips) { */
/*         std::vector<ParsedStopId> this_route_stops = my::stopset_id_to_stops(stopset_id); */
/*         stops.insert(this_route_stops.begin(), this_route_stops.end()); */
/*     } */

/*     // À ce stade, le set contient tous les stops. */

/*     std::vector<ParsedStopId> ranked_stops(stops.begin(), stops.end()); */
/*     std::unordered_map<ParsedStopId, size_t> stopidToRank; */
/*     for (size_t rank = 0; rank < ranked_stops.size(); ++rank) { */
/*         ParsedStopId stopid = ranked_stops[rank]; */
/*         stopidToRank.insert({stopid, rank}); */
/*     } */

/*     // à ce stade : */
/*     //  - ranked_stops permet de retrouver l'id d'un stop à partir de son rank */
/*     //  - stopidToRank permet de retrouver le rank d'un stop à partir de son id */
/*     return {ranked_stops, stopidToRank}; */
/* } */

/* // vérifie que tous les trips d'un stopset donné ont bien la même route : */
/* inline void assert_identical_stopset_routes(ad::cppgtfs::gtfs::Feed const& feed, */
/*                                             std::map<RouteID, std::set<TripID>> const& route_to_trips) { */
/*     for (auto[stopset_id, trips] : route_to_trips) { */
/*         RouteId reference_route_id = route_id_from_trip_id(feed, *trips.begin()); */

/*         auto is_mismatch = [&reference_route_id, &feed](auto trip_id) { */
/*             return route_id_from_trip_id(feed, trip_id) != reference_route_id; */
/*         }; */

/*         auto mismatching_trip = find_if(trips.cbegin(), trips.cend(), is_mismatch); */
/*         if (mismatching_trip != trips.cend()) { */
/*             std::cout << "This stopset has more than 1 real route : " << stopset_id << std::endl; */
/*             std::cout << "Reference route_id = " << reference_route_id << std::endl; */
/*             std::cout << "Mismatching trip = " << *mismatching_trip << std::endl; */
/*             std::cout << "Mismatching trip's route id = " << route_id_from_trip_id(feed, *mismatching_trip) */
/*                       << std::endl; */
/*             throw std::runtime_error("failed to assert_identical_stopset_routes"); */
/*         } */
/*     } */
/* } */

/* inline ad::cppgtfs::gtfs::Trip const& get_trip(ad::cppgtfs::gtfs::Feed const& feed, TripID const& trip_id) { */
/*     auto trip_ptr = feed.getTrips().get(trip_id); */
/*     if (trip_ptr == 0) { */
/*         std::ostringstream oss; */
/*         oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)"; */
/*         throw std::runtime_error(oss.str()); */
/*     } */
/*     return *trip_ptr; */
/* } */

}  // namespace my
