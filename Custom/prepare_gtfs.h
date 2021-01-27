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

    RouteID routeId{};

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        routeId.append(stop.getId());
        routeId.append("+");
    }

    // remove final '+' :
    return routeId.substr(0, routeId.size() - 1);
}

inline std::vector<StopID> routeToStops(RouteID const& route) {
    std::vector<StopID> stops;
    std::string token;
    std::istringstream iss(route);
    while (std::getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

inline std::map<RouteID, std::set<TripID>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // ULTRA uses "scientific" routes, but feed only has GTFS routes.
    // This functions partitions trips amongst their (scientific) route.
    // Two trips will have the same route IF they have excatly the same stops.

    std::map<RouteID, std::set<TripID>> routeToTrips;

    for (auto const & [ tripId, tripPtr ] : feed.getTrips()) {
        auto& trip = *(tripPtr);
        RouteID thisRouteId = build_route_id(trip);
        routeToTrips[thisRouteId].emplace(tripId);
    }
    return routeToTrips;
}

inline std::pair<std::vector<RouteID>, std::unordered_map<RouteID, size_t>> rankRoutes(
    std::map<RouteID, std::set<TripID>> const& routeToTrips) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank allows routes to be stored in a vector)

    size_t routeRank = 0;
    std::vector<RouteID> rankedRoutes;
    std::unordered_map<RouteID, size_t> routeToRank;

    for (auto ite = routeToTrips.cbegin(); ite != routeToTrips.cend(); ++ite) {
        RouteID const& routeId = ite->first;
        rankedRoutes.push_back(routeId);
        routeToRank.insert({routeId, routeRank++});
    }

    // Here :
    //   - rankedRoutes associates a rank to a route
    //   - routeToRank associates a route to its rank
    return {rankedRoutes, routeToRank};
}

inline std::pair<std::vector<StopID>, std::unordered_map<StopID, size_t>> rankStops(
    std::map<RouteID, std::set<TripID>> const& routeToTrips) {
    // this function ranks the stops (stops not used in routes are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank allows stops to be stored in a vector)

    // first, identify the stops that are used by at least one partitioned route :
    std::set<StopID> useful_stops;
    for (auto & [ route_id, _ ] : routeToTrips) {
        std::vector<StopID> this_route_stops = my::routeToStops(route_id);
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
