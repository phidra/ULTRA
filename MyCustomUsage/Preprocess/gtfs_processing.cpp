#include <numeric>

#include "gtfs_processing.h"

using namespace std;

namespace my::preprocess {

static RouteLabel _buildRouteLabel(ad::cppgtfs::gtfs::Trip const& trip) {
    // build the label of the trip's route (scientific route, see below).
    // A route label is just the concatenation of its stop's ids :
    //     32+33+34+122+123+125+126
    // precondition : no stopID constains the delimiter '+'

    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
    }

    RouteLabel routeId{};

#ifndef NDEBUG
    int previousDepartureTime = -1;
#endif

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        routeId.append(stop.getId());
        routeId.append("+");

#ifndef NDEBUG
        // verifying that stop times are properly ordered :
        int currentDepartureTime = stoptime.getDepartureTime().seconds();
        if (currentDepartureTime <= previousDepartureTime) {
            throw runtime_error( "ERROR : stoptimes are not properly ordered !");
        }
        previousDepartureTime = currentDepartureTime;
#endif
    }

    // remove final '+' :
    return routeId.substr(0, routeId.size() - 1);
}

map<RouteLabel, set<OrderableTripLabel>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // This function partitions the trips of the GTFS feed, according to their stops.
    // All The trips with exactly the same set of stops are grouped into a (scientific) 'route'.

    // WARNING : there are two mismatching definitions of the word "route" :
    //  - what scientific papers calls "route" is a particular set of stops
    //    in particular, if two trips travel between exactly the same stops, they belong to the same route.
    //  - what GTFS standard (and thus cppgtfs) calls "route" is just a given structure associated to a trip
    //    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
    //    even if they don't use exactly the same set of stops
    //
    // BEWARE : the "routes" returned by cppgtfs are not the scientific ones !
    //
    // In general, in ULTRA code (and in code building ULTRA data), the "routes" are the scientific ones.
    // Particularly, the "routes" returned by the present function are scientific routes.
    // A route is identified by its label.
    // Two trips will have the same route label IF they have excatly the same sequence of stops.

    map<RouteLabel, set<OrderableTripLabel>> routeToTrips;

    for (auto const & [ tripId, tripPtr ] : feed.getTrips()) {
        auto& trip = *(tripPtr);

        // in the set, all the trips of a given route are ordered by their departure times
        int tripDepartureTimeSeconds = trip.getStopTimes().begin()->getDepartureTime().seconds();

        RouteLabel thisRouteId = _buildRouteLabel(trip);
        routeToTrips[thisRouteId].emplace(tripDepartureTimeSeconds, tripId);
    }

    return routeToTrips;
}

bool checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, map<RouteLabel, set<OrderableTripLabel>> const& partition) {
    // checks that the agregation of the trips of all routes have the same number of trips than feed
    auto nbTripsInFeed = feed.getTrips().size();
    int nbTripsInPartitions = accumulate(
        partition.cbegin(),
        partition.cend(),
        0,
        [](int acc, auto const& routeToTrips) { return acc + routeToTrips.second.size(); }
    );
    return nbTripsInFeed == nbTripsInPartitions;
}

pair<vector<RouteLabel>, unordered_map<RouteLabel, size_t>> rankRoutes(
    map<RouteLabel, set<OrderableTripLabel>> const& routeToTrips) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank will be used to store the routes in a vector)

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
    //   - routeToRank allows to retrieve the rank of a given route
    return {rankedRoutes, routeToRank};
}

pair<vector<StopLabel>, unordered_map<StopLabel, size_t>> rankStops(
    map<RouteLabel, set<OrderableTripLabel>> const& routeToTrips) {
    // this function ranks the stops (and filter them : stops not used in at least a route are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank will be used to store the stops in a vector)

    // first, identify the stops that are used by at least one route :
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
    //   - stopToRank allows to retrieve the rank of a given stop
    return {rankedStops, stopToRank};
}

vector<StopLabel> routeToStops(RouteLabel const& route) {
    // from a given routeLabel, this functions builds back the list of its stops :
    vector<StopLabel> stops;
    string token;
    istringstream iss(route);
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

}
