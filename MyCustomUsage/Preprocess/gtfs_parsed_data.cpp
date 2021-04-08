#include <numeric>

#include "gtfs_parsed_data.h"

using namespace std;

namespace my::preprocess {


static ad::cppgtfs::gtfs::Stop const& getStop(ad::cppgtfs::gtfs::Feed const& feed, my::preprocess::StopLabel const& stopLabel) {
    auto stopPtr = feed.getStops().get(stopLabel);
    if (stopPtr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get stop with label '" << stopLabel << "' (stopPtr is 0)";
        throw runtime_error(oss.str());
    }

    return *stopPtr;
}

static map<RouteLabel, set<OrderableTripLabel>> _partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // This function partitions the trips of the GTFS feed, according to their stops.
    // All The trips with exactly the same set of stops are grouped into a (scientific) 'route'.
    // Once partitionned, a (scientific) route is identified by its RouteLabel.
    // Two trips will have the same route label IF they have excatly the same sequence of stops.

    map<RouteLabel, set<OrderableTripLabel>> routeToTrips;

    for (auto const & [ tripId, tripPtr ] : feed.getTrips()) {
        auto& trip = *(tripPtr);

        // in the set, all the trips of a given route are ordered by their departure times
        int tripDepartureTimeSeconds = trip.getStopTimes().begin()->getDepartureTime().seconds();

        RouteLabel thisRouteId = RouteLabel::fromTrip(trip);
        routeToTrips[thisRouteId].emplace(tripDepartureTimeSeconds, tripId);
    }

    return routeToTrips;
}

[[maybe_unused]] static bool _checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, map<RouteLabel, set<OrderableTripLabel>> const& partition) {
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

static pair<vector<RouteLabel>, unordered_map<RouteLabel, size_t>> _rankRoutes(
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
    return {move(rankedRoutes), move(routeToRank)};
}

static pair<vector<ParsedStop>, unordered_map<StopLabel, size_t>> _rankStops(
    map<RouteLabel, set<OrderableTripLabel>> const& routeToTrips, ad::cppgtfs::gtfs::Feed const& feed) {

    // this function ranks the stops (and filter them : stops not used in at least a route are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank will be used to store the stops in a vector)

    // first, identify the stops that are used by at least one route :
    set<StopLabel> usefulStops;
    for (auto & [ routeLabel, _ ] : routeToTrips) {
        vector<StopLabel> thisRouteStops = routeLabel.toStops();
        usefulStops.insert(thisRouteStops.begin(), thisRouteStops.end());
    }

    // then, rank them :
    size_t rank = 0;
    vector<ParsedStop> rankedStops;
    unordered_map<StopLabel, size_t> stopToRank;
    for (auto& stopLabel: usefulStops) {
        Stop const& stop = getStop(feed, stopLabel);
        rankedStops.emplace_back(stop.getName(), stop.getLat(), stop.getLng());
        stopToRank.insert({stopLabel, rank});
        ++rank;
    }

    // Here :
    //   - rankedStops associates a rank to a stop
    //   - stopToRank allows to retrieve the rank of a given stop
    return {move(rankedStops), move(stopToRank)};
}

GtfsParsedData::GtfsParsedData(ad::cppgtfs::gtfs::Feed const& feed) {
    routeToTrips = _partitionTripsInRoutes(feed);

#ifndef NDEBUG
    bool isPartitionConsistent = _checkRoutePartitionConsistency(feed, routeToTrips);
    if (!isPartitionConsistent) {
        ostringstream oss;
        oss << "ERROR : number of trips after partitioning by route is not the same than number of trips in feed (=" << feed.getTrips().size() << ")";
        throw runtime_error(oss.str());
    }
#endif

    tie(rankedRoutes, routeToRank) = _rankRoutes(routeToTrips);
    tie(rankedStops, stopToRank) = _rankStops(routeToTrips, feed);
}

}
