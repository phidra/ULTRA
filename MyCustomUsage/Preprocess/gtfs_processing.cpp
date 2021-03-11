#include <numeric>

#include "gtfs_processing.h"

using namespace std;

namespace my::preprocess {

static RouteLabel buildRouteLabel(ad::cppgtfs::gtfs::Trip const& trip) {
    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
    }

    RouteLabel routeId{};

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        routeId.append(stop.getId());
        routeId.append("+");
    }

    // remove final '+' :
    return routeId.substr(0, routeId.size() - 1);
}

vector<StopLabel> routeToStops(RouteLabel const& route) {
    vector<StopLabel> stops;
    string token;
    istringstream iss(route);
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

map<RouteLabel, set<TripLabel>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // ULTRA uses "scientific" routes, but feed only has GTFS routes.
    // This functions partitions trips amongst their (scientific) route, identified by its label.
    // Two trips will have the same route label IF they have excatly the same sequence of stops.

    map<RouteLabel, set<TripLabel>> routeToTrips;

    for (auto const & [ tripId, tripPtr ] : feed.getTrips()) {
        auto& trip = *(tripPtr);
        RouteLabel thisRouteId = buildRouteLabel(trip);
        routeToTrips[thisRouteId].emplace(tripId);
    }
    return routeToTrips;
}

bool checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, map<RouteLabel, set<TripLabel>> const& partition) {
    // checks that the agregation of the trips of all routes have the same number of trips than feed
    auto nbTripsInFeed = feed.getTrips().size();
    int nbTripsInPartitions = std::accumulate(
        partition.cbegin(),
        partition.cend(),
        0,
        [](int acc, auto const& routeToTrips) { return acc + routeToTrips.second.size(); }
    );
    return nbTripsInFeed == nbTripsInPartitions;
}

pair<vector<RouteLabel>, unordered_map<RouteLabel, size_t>> rankRoutes(
    map<RouteLabel, set<TripLabel>> const& routeToTrips) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank allows routes to be stored in a vector)

    size_t routeRank = 0;
    vector<RouteLabel> rankedRoutes;
    unordered_map<RouteLabel, size_t> routeToRank;

    for (auto ite = routeToTrips.cbegin(); ite != routeToTrips.cend(); ++ite) {
        RouteLabel const& routeId = ite->first;
        rankedRoutes.push_back(routeId);
        routeToRank.insert({routeId, routeRank++});
    }

    // Here :
    //   - rankedRoutes associates a rank to a route
    //   - routeToRank associates a route to its rank
    return {rankedRoutes, routeToRank};
}

pair<vector<StopLabel>, unordered_map<StopLabel, size_t>> rankStops(
    map<RouteLabel, set<TripLabel>> const& routeToTrips) {
    // this function ranks the stops (stops not used in routes are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank allows stops to be stored in a vector)

    // first, identify the stops that are used by at least one partitioned route :
    set<StopLabel> usefulStops;
    for (auto & [ route_id, _ ] : routeToTrips) {
        vector<StopLabel> thisRouteStops = routeToStops(route_id);
        usefulStops.insert(thisRouteStops.begin(), thisRouteStops.end());
    }

    // then, rank them :
    vector<StopLabel> rankedStops(usefulStops.begin(), usefulStops.end());
    unordered_map<StopLabel, size_t> stopToRank;
    for (size_t rank = 0; rank < rankedStops.size(); ++rank) {
        StopLabel stoplabel = rankedStops[rank];
        stopToRank.insert({stoplabel, rank});
    }

    // Here :
    //   - rankedStops associates a rank to a stop
    //   - stopToRank associates a stop to its rank
    return {rankedStops, stopToRank};
}

}
