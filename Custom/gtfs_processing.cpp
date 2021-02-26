#include "gtfs_processing.h"

using namespace std;

namespace my {

static RouteID build_route_id(ad::cppgtfs::gtfs::Trip const& trip) {
    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
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

vector<StopID> routeToStops(RouteID const& route) {
    vector<StopID> stops;
    string token;
    istringstream iss(route);
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

map<RouteID, set<TripID>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // ULTRA uses "scientific" routes, but feed only has GTFS routes.
    // This functions partitions trips amongst their (scientific) route.
    // Two trips will have the same route IF they have excatly the same stops.

    map<RouteID, set<TripID>> routeToTrips;

    for (auto const & [ tripId, tripPtr ] : feed.getTrips()) {
        auto& trip = *(tripPtr);
        RouteID thisRouteId = build_route_id(trip);
        routeToTrips[thisRouteId].emplace(tripId);
    }
    return routeToTrips;
}

pair<vector<RouteID>, unordered_map<RouteID, size_t>> rankRoutes(
    map<RouteID, set<TripID>> const& routeToTrips) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank allows routes to be stored in a vector)

    size_t routeRank = 0;
    vector<RouteID> rankedRoutes;
    unordered_map<RouteID, size_t> routeToRank;

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

pair<vector<StopID>, unordered_map<StopID, size_t>> rankStops(
    map<RouteID, set<TripID>> const& routeToTrips) {
    // this function ranks the stops (stops not used in routes are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank allows stops to be stored in a vector)

    // first, identify the stops that are used by at least one partitioned route :
    set<StopID> usefulStops;
    for (auto & [ route_id, _ ] : routeToTrips) {
        vector<StopID> thisRouteStops = routeToStops(route_id);
        usefulStops.insert(thisRouteStops.begin(), thisRouteStops.end());
    }

    // then, rank them :
    vector<StopID> rankedStops(usefulStops.begin(), usefulStops.end());
    unordered_map<StopID, size_t> stopToRank;
    for (size_t rank = 0; rank < rankedStops.size(); ++rank) {
        StopID stopid = rankedStops[rank];
        stopToRank.insert({stopid, rank});
    }

    // Here :
    //   - rankedStops associates a rank to a stop
    //   - stopToRank associates a stop to its rank
    return {rankedStops, stopToRank};
}

}  // namespace my
