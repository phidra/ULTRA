#include <iostream>
#include <numeric>
#include <algorithm>

#include "gtfs_to_ultra_binary.h"
#include "gtfs_processing.h"
#include "Common/autodeletefile.h"

#include "ad/cppgtfs/Parser.h"

using namespace std;

// This code helps to build the RAPTOR binary expected by ULTRA, from a given GTFS feed.

namespace my {

static ad::cppgtfs::gtfs::Stop const& getStop(ad::cppgtfs::gtfs::Feed const& feed, my::StopID const& stopId) {
    auto stopPtr = feed.getStops().get(stopId);
    if (stopPtr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get stop with id '" << stopId << "' (stopPtr is 0)";
        throw runtime_error(oss.str());
    }

    return *stopPtr;
}

static ad::cppgtfs::gtfs::Trip const& getTrip(ad::cppgtfs::gtfs::Feed const& feed, my::TripID const& tripId) {
    auto tripPtr = feed.getTrips().get(tripId);
    if (tripPtr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << tripId << "' (tripPtr is 0)";
        throw runtime_error(oss.str());
    }
    return *tripPtr;
}

static vector<RAPTOR::Route> build_routeData(vector<RouteID> const& rankedRoutes) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(rankedRoutes.size());
    // note : The route name is NOT its rank, but its id, i.e. the concatenation of its stops
    transform(rankedRoutes.begin(), rankedRoutes.end(), back_inserter(routeData),
              [](auto& routeID) { return RAPTOR::Route(routeID); });
    return routeData;
}

static vector<RAPTOR::Stop> build_stopData(vector<my::StopID> const& rankedStops,
                                                ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::Stop> stopData(rankedStops.size());
    for (size_t rank = 0; rank < rankedStops.size(); ++rank) {
        my::StopID stopId = rankedStops[rank];
        Stop const& stop = getStop(feed, stopId);
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData[rank] = RAPTOR::Stop{stop.getName(), location};
    }
    return stopData;
}

static pair<vector<::StopId>, vector<size_t>> build_stopIdsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<my::StopID, size_t> const& stopToRank) {
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    vector<::StopId> stopIds;

    size_t currentRouteFirstStop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID routeId = routeData[routeRank].name;
        vector<my::StopID> stopsOfCurrentRoute = my::routeToStops(routeId);
        transform(stopsOfCurrentRoute.cbegin(), stopsOfCurrentRoute.cend(), back_inserter(stopIds),
                  [&stopToRank](my::StopID const& stopid) { return ::StopId{static_cast<u_int32_t>(stopToRank.at(stopid))}; });

        firstStopIdOfRoute[routeRank] = currentRouteFirstStop;
        currentRouteFirstStop += stopsOfCurrentRoute.size();
    }

    // From now on :
    //      routeRank = number of routes
    //      currentRouteFirstStop = number of stops in all the routes
    // Setting past-the-end stopIDs :
    firstStopIdOfRoute[routeRank] = currentRouteFirstStop;
    return {stopIds, firstStopIdOfRoute};
}

static pair<vector<RAPTOR::StopEvent>, vector<size_t>> build_stopEventsRelated(
    vector<RAPTOR::Route> const& routeData,
    map<my::RouteID, set<my::TripID>> const& routeToTrips,
    ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::StopEvent> stopEvents;
    vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t currentRouteFirstStopEvent = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID routeId = routeData[routeRank].name;

        size_t nbStopEventsInThisRoute = 0;

        // each route "contains" (=is associated with) several trips :
        auto const& tripsOfCurrentRoute = routeToTrips.at(routeId);
        for (auto& tripId : tripsOfCurrentRoute) {
            auto& trip = getTrip(feed, tripId);

            // each trip contains a range of stopevents :
            for (auto const& stoptime : trip.getStopTimes()) {
                int departureTime = stoptime.getDepartureTime().seconds();
                int arrivalTime = stoptime.getArrivalTime().seconds();
                stopEvents.emplace_back(arrivalTime, departureTime);
            }

            size_t nbStopEventsInThisTrip = trip.getStopTimes().size();
            nbStopEventsInThisRoute += nbStopEventsInThisTrip;
        }

        firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;
        currentRouteFirstStopEvent += nbStopEventsInThisRoute;
    }

    // From now on :
    //      routeRank = number of routes
    //      currentRouteFirstStopEvent = number of stopevents in all the routes
    // Setting past-the-end stopEvents :
    firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;

    return {stopEvents, firstStopEventOfRoute};
}

static pair<vector<RAPTOR::RouteSegment>, vector<size_t>> convert_routeSegmentsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<my::StopID, size_t> const& stopToRank,
    unordered_map<my::RouteID, size_t> const& routeToRank,
    map<my::RouteID, set<my::TripID>> const& routeToTrips) {
    // building an intermediate structure that associates a stopRank to all its routes
    // for a given stop, this structures stores some pairs {route + stop index in this route}

    auto nb_stops = stopToRank.size();
    vector<vector<pair<my::RouteID, int>>> routesUsingAStop(nb_stops);

    for (auto& route : routeData) {
        my::RouteID routeId = route.name;
        vector<my::StopID> stopsOfThisRoute = my::routeToStops(routeId);

        for (size_t stopIndex = 0; stopIndex < stopsOfThisRoute.size(); ++stopIndex) {
            my::StopID const& stopId = stopsOfThisRoute[stopIndex];
            size_t stopRank = stopToRank.at(stopId);

            vector<pair<my::RouteID, int>>& routesUsingThisStop = routesUsingAStop[stopRank];
            routesUsingThisStop.emplace_back(routeId, stopIndex);
        }
    }

    // From now on, for each stop, we know the routes that uses it

    // TODO = check that each route only appears once for a given stop

    vector<size_t> firstRouteSegmentOfStop(stopToRank.size() + 1);
    vector<RAPTOR::RouteSegment> routeSegments;

    size_t currentStopFirstRouteSegment = 0;

    for (size_t stopRank = 0; stopRank < routesUsingAStop.size(); ++stopRank) {
        auto& routesUsingThisStop = routesUsingAStop[stopRank];

        for (auto & [ routeId, stopIndexInThisRoute ] : routesUsingThisStop) {
            auto routeRank = static_cast<u_int32_t>(routeToRank.at(routeId));
            routeSegments.emplace_back(::RouteId{routeRank}, StopIndex{static_cast<u_int32_t>(stopIndexInThisRoute)});
        }

        firstRouteSegmentOfStop[stopRank] = currentStopFirstRouteSegment;
        currentStopFirstRouteSegment += routesUsingThisStop.size();
    }

    // From now on :
    //      currentStopFirstRouteSegment = total number of routesgments
    // Setting past-the-end routeSegments :
    auto nbRoutes = routesUsingAStop.size();
    firstRouteSegmentOfStop[nbRoutes] = currentStopFirstRouteSegment;

    return {routeSegments, firstRouteSegmentOfStop};
}

static void fillFromFeed(ad::cppgtfs::gtfs::Feed const& feed, my::UltraGtfsData& toFill) {
    // prepare GTFS data :
    auto routeToTrips = partitionTripsInRoutes(feed);
    bool isPartitionConsistent = my::checkRoutePartitionConsistency(feed, routeToTrips);
    if (!isPartitionConsistent) {
        ostringstream oss;
        oss << "ERROR : number of trips after partitioning by route is not the same than number of trips in feed (=" << feed.getTrips().size() << ")";
        throw runtime_error(oss.str());
    }
    auto[rankedRoutes, routeToRank] = rankRoutes(routeToTrips);
    auto[rankedStops, stopToRank] = rankStops(routeToTrips);

    // from now on :
    //  - routes of GTFS data are not used anymore (they are replaced with a partition of trips by routes
    //  - only the stops that appear in at least one trip are used
    //  - a route (or a stop) can be identified with its RouteID/StopID or its rank
    //  - the conversion between ID<->rank is done with the above structures

    toFill.routeData = build_routeData(rankedRoutes);
    toFill.stopData = build_stopData(rankedStops, feed);
    tie(toFill.stopIds, toFill.firstStopIdOfRoute) = build_stopIdsRelated(toFill.routeData, stopToRank);
    tie(toFill.stopEvents, toFill.firstStopEventOfRoute) = build_stopEventsRelated(toFill.routeData, routeToTrips, feed);
    tie(toFill.routeSegments, toFill.firstRouteSegmentOfStop) =
        convert_routeSegmentsRelated(toFill.routeData, stopToRank, routeToRank, routeToTrips);

    // STUB : according to some comments in ULTRARAPTOR.h, buffer times have to be implicit :
    toFill.implicitDepartureBufferTimes = true;
    toFill.implicitArrivalBufferTimes = true;
}


my::UltraGtfsData::UltraGtfsData(string const& gtfsFolder) {
    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfsFolder);
    fillFromFeed(feed, *this);
}


void my::UltraGtfsData::dump(string const& filename) const {
    IO::serialize(filename, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
}


bool my::UltraGtfsData::checkSerializationIdempotence() const {
    my::AutoDeleteTempFile tmpfile;

    // serializing in a temporary file :
    dump(tmpfile.file);

    // unserializing, to check that serialization+deserialization is idempotent :
    vector<size_t> freshFirstRouteSegmentOfStop;
    vector<size_t> freshFirstStopIdOfRoute;
    vector<size_t> freshFirstStopEventOfRoute;
    vector<RAPTOR::RouteSegment> freshRouteSegments;
    vector<::StopId> freshStopIds;
    vector<RAPTOR::StopEvent> freshStopEvents;
    vector<RAPTOR::Stop> freshStopData;
    vector<RAPTOR::Route> freshRouteData;
    bool freshImplicitDepartureBufferTimes;
    bool freshImplicitArrivalBufferTimes;

    IO::deserialize(tmpfile.file, freshFirstRouteSegmentOfStop, freshFirstStopIdOfRoute, freshFirstStopEventOfRoute, freshRouteSegments, freshStopIds, freshStopEvents, freshStopData, freshRouteData, freshImplicitDepartureBufferTimes, freshImplicitArrivalBufferTimes);

    bool areFirstRouteSegmentOfStopEqual = firstRouteSegmentOfStop == freshFirstRouteSegmentOfStop;
    bool areFirstStopIdOfRouteEqual = firstStopIdOfRoute == freshFirstStopIdOfRoute;
    bool areFirstStopEventOfRouteEqual = firstStopEventOfRoute == freshFirstStopEventOfRoute;
    bool areRouteSegmentsEqual = equal(
        routeSegments.begin(),
        routeSegments.end(),
        freshRouteSegments.begin(),
        [](auto const& x, auto const& y) { return x.routeId == y.routeId && x.stopIndex == y.stopIndex; }
    );
    bool areStopIdsEqual = stopIds == freshStopIds;
    bool areStopEventsEqual = equal(
        stopEvents.begin(),
        stopEvents.end(),
        freshStopEvents.begin(),
        [](auto const& x, auto const& y) { return x.arrivalTime == y.arrivalTime && x.departureTime == y.departureTime; }
    );
    bool areStopDataEqual = equal(
        stopData.begin(),
        stopData.end(),
        freshStopData.begin(),
        [](auto const& x, auto const& y) { return x.name == y.name && x.coordinates == y.coordinates && x.minTransferTime == y.minTransferTime; }
    );
    bool areRouteDataEqual = equal(
        routeData.begin(),
        routeData.end(),
        freshRouteData.begin(),
        [](auto const& x, auto const& y) { return x.name == y.name && x.type == y.type; }
    );
    bool areImplicitDepartureBufferTimesEqual = implicitDepartureBufferTimes == freshImplicitDepartureBufferTimes;
    bool areImplicitArrivalBufferTimesEqual = implicitArrivalBufferTimes == freshImplicitArrivalBufferTimes;


    cout << "DETAILS : is serialization + deserialization idempotent ?" << endl;
    cout << boolalpha;
    cout << areFirstRouteSegmentOfStopEqual << endl;
    cout << areFirstStopIdOfRouteEqual << endl;
    cout << areFirstStopEventOfRouteEqual << endl;
    cout << areRouteSegmentsEqual << endl;
    cout << areStopIdsEqual << endl;
    cout << areStopEventsEqual << endl;
    cout << areStopDataEqual << endl;
    cout << areRouteDataEqual << endl;
    cout << areImplicitDepartureBufferTimesEqual << endl;
    cout << areImplicitArrivalBufferTimesEqual << endl;


    return (
        areFirstRouteSegmentOfStopEqual &&
        areFirstStopIdOfRouteEqual &&
        areFirstStopEventOfRouteEqual &&
        areRouteSegmentsEqual &&
        areStopIdsEqual &&
        areStopEventsEqual &&
        areStopDataEqual &&
        areRouteDataEqual &&
        areImplicitDepartureBufferTimesEqual &&
        areImplicitArrivalBufferTimesEqual
    );
}

void UltraGtfsData::serialize(const string& fileName) const {
    IO::serialize(fileName, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
}

}  // namespace my

