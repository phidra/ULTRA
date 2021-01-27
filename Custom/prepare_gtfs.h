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
//  - StopId -> this is an ULTRA structure
//  - StopID -> this is a prepared GTFS structure (built by this code)

// this is the "scientific" route used in ULTRA code :
using RouteID = std::string;
// this is the id of a "route" in the GTFS data (barely used in ULTRA code) :
using GtfsRouteID = std::string;

using TripID = std::string;
using RouteId = std::string;

using StopID = std::string;

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

inline std::vector<StopID> route_to_stops(RouteID const& route) {
    std::vector<StopID> stops;
    std::string token;
    std::istringstream iss(route);
    while (std::getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

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
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
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

inline std::pair<std::vector<StopID>, std::unordered_map<StopID, size_t>> rank_stops(
    std::map<RouteID, std::set<TripID>> const& route_to_trips) {
    // this function ranks the stops (stops not used in routes are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank allows stops to be stored in a vector)

    // first, identify the stops that are used by at least one partitioned route :
    std::set<StopID> useful_stops;
    for (auto & [ route_id, _ ] : route_to_trips) {
        std::vector<StopID> this_route_stops = my::route_to_stops(route_id);
        useful_stops.insert(this_route_stops.begin(), this_route_stops.end());
    }

    // then, rank them :
    std::vector<StopID> ranked_stops(useful_stops.begin(), useful_stops.end());
    std::unordered_map<StopID, size_t> stop_to_rank;
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        StopID stopid = ranked_stops[rank];
        stop_to_rank.insert({stopid, rank});
    }

    // Here :
    //   - ranked_stops associates a rank to a stop
    //   - stop_to_rank associates a stop to its rank
    return {ranked_stops, stop_to_rank};
}

}  // namespace my
