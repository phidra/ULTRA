#include <iostream>
#include <numeric>
#include <algorithm>

#include "gtfs_to_ultra_binary.h"
#include "prepare_gtfs.h"

using namespace std;

// This code helps to build the RAPTOR binary expected by ULTRA, from a given GTFS feed.
// note : ULTRA code is not safe to use in multiple translation units, thus all the conversion code is in this header...


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

vector<RAPTOR::Route> build_routeData(map<my::RouteID, set<my::TripID>> const& routeToTrips) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(routeToTrips.size());
    // note : The route name is NOT its rank, but its id, i.e. the concatenation of its stops
    transform(routeToTrips.begin(), routeToTrips.end(), back_inserter(routeData),
              [](auto& routeToTrip) { return RAPTOR::Route(routeToTrip.first); });
    return routeData;
}

vector<RAPTOR::Stop> build_stopData(vector<my::StopID> const& rankedStops,
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

pair<vector<StopId>, vector<size_t>> build_stopIdsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<my::StopID, size_t> const& stopToRank) {
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    vector<StopId> stopIds;

    size_t currentRouteFirstStop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID routeId = routeData[routeRank].name;
        vector<my::StopID> stopsOfCurrentRoute = my::routeToStops(routeId);
        transform(stopsOfCurrentRoute.cbegin(), stopsOfCurrentRoute.cend(), back_inserter(stopIds),
                  [&stopToRank](my::StopID const& stopid) { return StopId{static_cast<u_int32_t>(stopToRank.at(stopid))}; });

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

pair<vector<RAPTOR::StopEvent>, vector<size_t>> build_stopEventsRelated(
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

pair<vector<RAPTOR::RouteSegment>, vector<size_t>> convert_routeSegmentsRelated(
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

void convert_gtfs_to_ultra_binary(ad::cppgtfs::gtfs::Feed const& feed, string const& outputFileName, bool checkIdempotence) {
    // prepare GTFS data :
    auto routeToTrips = partitionTripsInRoutes(feed);
    auto[rankedRoutes, routeToRank] = rankRoutes(routeToTrips);
    auto[rankedStops, stopToRank] = rankStops(routeToTrips);

    // after this preparation step, from now on :
    //  - routes of GTFS data are not used anymore (they are replaced with a partition of stops, cf. prepare_gtfs.h)
    //  - only the stops that appear in at least one trip are used
    //  - a route (or a stop) can be identified with its RouteID/StopID or its rank
    //  - the conversion between ID<->rank is done with the above structures

    // routeData :
    vector<RAPTOR::Route> routeData = build_routeData(routeToTrips);
    cout << "Here, routeData contains : " << routeData.size() << " items." << endl;
    int routeCounter = 0;
    for (auto& route : routeData) {
        if (routeCounter++ <= 8) {
            cout << "ROUTE = " << route << endl;
        }
    }

    // stopData :
    vector<RAPTOR::Stop> stopData = build_stopData(rankedStops, feed);
    cout << "Here, stopData contains : " << stopData.size() << " items." << endl;
    int stopCounter = 0;
    for (auto& stop : stopData) {
        if (stopCounter++ <= 8) {
            cout << "STOP = " << stop << endl;
        }
    }

    // stopIds + firstStopIdOfRoute :
    auto[stopIds, firstStopIdOfRoute] = build_stopIdsRelated(routeData, stopToRank);
    cout << "Here, stopIds contains : " << stopIds.size() << " items." << endl;
    cout << "Here, firstStopIdOfRoute contains : " << firstStopIdOfRoute.size() << " items." << endl;
    int otherRouteCounter = 0;
    for (auto idx : firstStopIdOfRoute) {
        if (otherRouteCounter++ <= 8) {
            cout << "First stop id of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopIdOfRoute is " << firstStopIdOfRoute.back() << endl;

    // stopEvents + firstStopEventOfRoute :
    auto[stopEvents, firstStopEventOfRoute] = build_stopEventsRelated(routeData, routeToTrips, feed);
    cout << "Here, stopEvents contains : " << stopEvents.size() << " items." << endl;
    cout << "Here, firstStopEventOfRoute contains : " << firstStopEventOfRoute.size() << " items." << endl;
    int yetAnotherRouteCounter = 0;
    for (auto idx : firstStopEventOfRoute) {
        if (yetAnotherRouteCounter++ <= 8) {
            cout << "First stop event of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopEventOfRoute is " << firstStopEventOfRoute.back() << endl;

    // routeSegments + firstRouteSegmentOfStop
    auto[routeSegments, firstRouteSegmentOfStop] =
        convert_routeSegmentsRelated(routeData, stopToRank, routeToRank, routeToTrips);
    cout << "Here, routeSegments contains : " << routeSegments.size() << " items." << endl;
    cout << "Here, firstRouteSegmentOfStop contains : " << firstRouteSegmentOfStop.size() << " items." << endl;
    int lastRouteCounter = 0;
    for (auto idx : firstRouteSegmentOfStop) {
        if (lastRouteCounter++ <= 8) {
            cout << "First route segment of this stop = " << idx << endl;
        }
    }
    cout << "Last item of firstRouteSegmentOfStop is " << firstRouteSegmentOfStop.back() << endl;

    // serializing :
    bool implicitDepartureBufferTimes = false;
    bool implicitArrivalBufferTimes = false;
    IO::serialize(outputFileName, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);

    if (checkIdempotence) {

        // unserializing, to check that serialization+deserialization is idempotent :
        vector<size_t> freshFirstRouteSegmentOfStop;
        vector<size_t> freshFirstStopIdOfRoute;
        vector<size_t> freshFirstStopEventOfRoute;
        vector<RAPTOR::RouteSegment> freshRouteSegments;
        vector<StopId> freshStopIds;
        vector<RAPTOR::StopEvent> freshStopEvents;
        vector<RAPTOR::Stop> freshStopData;
        vector<RAPTOR::Route> freshRouteData;
        bool freshImplicitDepartureBufferTimes;
        bool freshImplicitArrivalBufferTimes;

        IO::deserialize(outputFileName, freshFirstRouteSegmentOfStop, freshFirstStopIdOfRoute, freshFirstStopEventOfRoute, freshRouteSegments, freshStopIds, freshStopEvents, freshStopData, freshRouteData, freshImplicitDepartureBufferTimes, freshImplicitArrivalBufferTimes);
        cout << "Is serialization + deserialization idempotent ?" << endl;
        cout << (firstRouteSegmentOfStop == freshFirstRouteSegmentOfStop) << endl;
        cout << (firstStopIdOfRoute == freshFirstStopIdOfRoute) << endl;
        cout << (firstStopEventOfRoute == freshFirstStopEventOfRoute) << endl;

        auto are_route_segments_equal = equal(
            routeSegments.begin(),
            routeSegments.end(),
            freshRouteSegments.begin(),
            [](auto const& left, auto const& right) { return left.routeId == right.routeId && left.stopIndex == right.stopIndex; }
        );

        cout << are_route_segments_equal << endl;
        cout << (stopIds == freshStopIds) << endl;

        auto are_stop_events_equal = equal(
            stopEvents.begin(),
            stopEvents.end(),
            freshStopEvents.begin(),
            [](auto const& left, auto const& right) { return left.arrivalTime == right.arrivalTime && left.departureTime == right.departureTime; }
        );

        cout << are_stop_events_equal << endl;

        auto are_stops_equal = equal(
            stopData.begin(),
            stopData.end(),
            freshStopData.begin(),
            [](auto const& left, auto const& right) { return left.name == right.name && left.coordinates == right.coordinates && left.minTransferTime == right.minTransferTime; }
        );
        cout << are_stops_equal << endl;


        auto are_routes_equal = equal(
            routeData.begin(),
            routeData.end(),
            freshRouteData.begin(),
            [](auto const& left, auto const& right) { return left.name == right.name && left.type == right.type; }
        );
        cout << are_routes_equal << endl;

        cout << (implicitDepartureBufferTimes == freshImplicitDepartureBufferTimes) << endl;
        cout << (implicitArrivalBufferTimes == freshImplicitArrivalBufferTimes) << endl;
        cout << "That's all folks !" << endl;
    }
}

}  // namespace my

