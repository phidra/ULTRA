#pragma once

#include <iostream>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../DataStructures/RAPTOR/Entities/RouteSegment.h"
#include "../Custom/prepare_gtfs.h"

// This code helps to build the RAPTOR binary expected by ULTRA, from a given GTFS feed.
// note : ULTRA code is not safe to use in multiple translation units, thus all the conversion code is in this header...

inline ad::cppgtfs::gtfs::Stop const& _getStop(ad::cppgtfs::gtfs::Feed const& feed, my::StopID const& stopId) {
    auto stopPtr = feed.getStops().get(stopId);
    if (stopPtr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get stop with id '" << stopId << "' (stopPtr is 0)";
        throw std::runtime_error(oss.str());
    }

    return *stopPtr;
}

inline ad::cppgtfs::gtfs::Trip const& _getTrip(ad::cppgtfs::gtfs::Feed const& feed, my::TripID const& tripId) {
    auto tripPtr = feed.getTrips().get(tripId);
    if (tripPtr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << tripId << "' (tripPtr is 0)";
        throw std::runtime_error(oss.str());
    }
    return *tripPtr;
}

inline std::vector<RAPTOR::Route> build_routeData(std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips) {
    std::vector<RAPTOR::Route> routeData;
    routeData.reserve(routeToTrips.size());
    // note : The route name is NOT its rank, but its id, i.e. the concatenation of its stops
    transform(routeToTrips.begin(), routeToTrips.end(), back_inserter(routeData),
              [](auto& routeToTrip) { return RAPTOR::Route(routeToTrip.first); });
    return routeData;
}

inline std::vector<RAPTOR::Stop> build_stopData(std::vector<my::StopID> const& rankedStops,
                                                ad::cppgtfs::gtfs::Feed const& feed) {
    std::vector<RAPTOR::Stop> stopData(rankedStops.size());
    for (size_t rank = 0; rank < rankedStops.size(); ++rank) {
        my::StopID stopId = rankedStops[rank];
        Stop const& stop = _getStop(feed, stopId);
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData[rank] = RAPTOR::Stop{stop.getName(), location};
    }
    return stopData;
}

inline std::pair<std::vector<StopId>, std::vector<size_t>> build_stopIdsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stopToRank) {
    std::vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    std::vector<StopId> stopIds;

    size_t currentRouteFirstStop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID routeId = routeData[routeRank].name;
        std::vector<my::StopID> stopsOfCurrentRoute = my::routeToStops(routeId);
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

inline std::pair<std::vector<RAPTOR::StopEvent>, std::vector<size_t>> build_stopEventsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips,
    ad::cppgtfs::gtfs::Feed const& feed) {
    std::vector<RAPTOR::StopEvent> stopEvents;
    std::vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t currentRouteFirstStopEvent = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID routeId = routeData[routeRank].name;

        size_t nbStopEventsInThisRoute = 0;

        // each route "contains" (=is associated with) several trips :
        auto const& tripsOfCurrentRoute = routeToTrips.at(routeId);
        for (auto& tripId : tripsOfCurrentRoute) {
            auto& trip = _getTrip(feed, tripId);

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

inline std::pair<std::vector<RAPTOR::RouteSegment>, std::vector<size_t>> convert_routeSegmentsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stopToRank,
    std::unordered_map<my::RouteID, size_t> const& routeToRank,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips) {
    // building an intermediate structure that associates a stopRank to all its routes
    // for a given stop, this structures stores some pairs {route + stop index in this route}

    auto nb_stops = stopToRank.size();
    std::vector<std::vector<std::pair<my::RouteID, int>>> routesUsingAStop(nb_stops);

    for (auto& route : routeData) {
        my::RouteID routeId = route.name;
        std::vector<my::StopID> stopsOfThisRoute = my::routeToStops(routeId);

        for (size_t stopIndex = 0; stopIndex < stopsOfThisRoute.size(); ++stopIndex) {
            my::StopID const& stopId = stopsOfThisRoute[stopIndex];
            size_t stopRank = stopToRank.at(stopId);

            std::vector<std::pair<my::RouteID, int>>& routesUsingThisStop = routesUsingAStop[stopRank];
            routesUsingThisStop.emplace_back(routeId, stopIndex);
        }
    }

    // From now on, for each stop, we know the routes that uses it

    // TODO = check that each route only appears once for a given stop

    std::vector<size_t> firstRouteSegmentOfStop(stopToRank.size() + 1);
    std::vector<RAPTOR::RouteSegment> routeSegments;

    size_t currentStopFirstRouteSegment = 0;

    for (size_t stopRank = 0; stopRank < routesUsingAStop.size(); ++stopRank) {
        auto& routesUsingThisStop = routesUsingAStop[stopRank];

        for (auto & [ routeId, stopIndexInThisRoute ] : routesUsingThisStop) {
            auto routeRank = static_cast<u_int32_t>(routeToRank.at(routeId));
            routeSegments.emplace_back(RouteId{routeRank}, StopIndex{static_cast<u_int32_t>(stopIndexInThisRoute)});
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
