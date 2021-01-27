#pragma once

#include <iostream>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../DataStructures/RAPTOR/Entities/RouteSegment.h"
#include "../Custom/prepare_gtfs.h"

// note : ULTRA code is not safe to use in multiple translation units, thus all the conversion code is in header...

inline ad::cppgtfs::gtfs::Stop const& _get_stop(ad::cppgtfs::gtfs::Feed const& feed, my::StopID const& stop_id) {
    auto stop_ptr = feed.getStops().get(stop_id);
    if (stop_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get stop with id '" << stop_id << "' (stop_ptr is 0)";
        throw std::runtime_error(oss.str());
    }

    return *stop_ptr;
}

inline ad::cppgtfs::gtfs::Trip const& _get_trip(ad::cppgtfs::gtfs::Feed const& feed, my::TripID const& trip_id) {
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)";
        throw std::runtime_error(oss.str());
    }
    return *trip_ptr;
}

inline std::vector<RAPTOR::Route> build_routeData(std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips) {
    std::vector<RAPTOR::Route> routeData;
    routeData.reserve(routeToTrips.size());
    // note : The route name is NOT its rank, but its id, i.e. the concatenation of its stops
    transform(routeToTrips.begin(), routeToTrips.end(), back_inserter(routeData),
              [](auto& stopset_to_trip) { return RAPTOR::Route(stopset_to_trip.first); });
    return routeData;
}

inline std::vector<RAPTOR::Stop> build_stopData(std::vector<my::StopID> const& ranked_stops,
                                                ad::cppgtfs::gtfs::Feed const& feed) {
    std::vector<RAPTOR::Stop> stopData(ranked_stops.size());
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        my::StopID stop_id = ranked_stops[rank];
        Stop const& stop = _get_stop(feed, stop_id);
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData[rank] = RAPTOR::Stop{stop.getName(), location};
    }
    return stopData;
}

inline std::pair<std::vector<StopId>, std::vector<size_t>> build_stopIdsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stop_to_rank) {
    std::vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    std::vector<StopId> stopIds;

    size_t current_route_first_stop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID route_id = routeData[routeRank].name;
        std::vector<my::StopID> stops_of_current_route = my::routeToStops(route_id);
        transform(stops_of_current_route.cbegin(), stops_of_current_route.cend(), back_inserter(stopIds),
                  [&stop_to_rank](my::StopID const& stopid) { return StopId{stop_to_rank.at(stopid)}; });

        firstStopIdOfRoute[routeRank] = current_route_first_stop;
        current_route_first_stop += stops_of_current_route.size();
    }

    // From now on :
    //      routeRank = number of routes
    //      current_route_first_stop = number of stops in all the routes
    // Setting past-the-end stopIDs :
    firstStopIdOfRoute[routeRank] = current_route_first_stop;

    return {stopIds, firstStopIdOfRoute};
}

inline std::pair<std::vector<RAPTOR::StopEvent>, std::vector<size_t>> build_stopEventsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips,
    ad::cppgtfs::gtfs::Feed const& feed) {
    std::vector<RAPTOR::StopEvent> stopEvents;
    std::vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t current_route_first_stopevent = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::RouteID route_id = routeData[routeRank].name;

        size_t nb_stopevents_in_this_route = 0;

        // each route "contains" (=is associated with) several trips :
        auto const& trips_of_current_route = routeToTrips.at(route_id);
        for (auto& trip_id : trips_of_current_route) {
            auto& trip = _get_trip(feed, trip_id);

            // each trip contains a range of stopevents :
            for (auto const& stoptime : trip.getStopTimes()) {
                int departure_time = stoptime.getDepartureTime().seconds();
                int arrival_time = stoptime.getArrivalTime().seconds();
                stopEvents.emplace_back(arrival_time, departure_time);
            }

            size_t nb_stopevents_in_this_trip = trip.getStopTimes().size();
            nb_stopevents_in_this_route += nb_stopevents_in_this_trip;
        }

        firstStopEventOfRoute[routeRank] = current_route_first_stopevent;
        current_route_first_stopevent += nb_stopevents_in_this_route;
    }

    // From now on :
    //      routeRank = number of routes
    //      current_route_first_stopevent = number of stopevents in all the routes
    // Setting past-the-end stopEvents :
    firstStopEventOfRoute[routeRank] = current_route_first_stopevent;

    return {stopEvents, firstStopEventOfRoute};
}

inline std::pair<std::vector<RAPTOR::RouteSegment>, std::vector<size_t>> convert_routeSegmentsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stop_to_rank,
    std::unordered_map<my::RouteID, size_t> const& routeToRank,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips) {
    // building an intermediate structure that associates a stop_rank to all its routes
    // for a given stop, this structures stores some pairs {route + stop index in this route}

    auto nb_stops = stop_to_rank.size();
    std::vector<std::vector<std::pair<my::RouteID, int>>> routes_using_a_stop(nb_stops);

    for (auto& route : routeData) {
        my::RouteID route_id = route.name;
        std::vector<my::StopID> stops_of_this_route = my::routeToStops(route_id);

        for (size_t stop_index = 0; stop_index < stops_of_this_route.size(); ++stop_index) {
            my::StopID const& stop_id = stops_of_this_route[stop_index];
            size_t stop_rank = stop_to_rank.at(stop_id);

            std::vector<std::pair<my::RouteID, int>>& routes_using_this_stop = routes_using_a_stop[stop_rank];
            routes_using_this_stop.emplace_back(route_id, stop_index);
        }
    }

    // From now on, for each stop, we know the routes that uses it

    // TODO = check that each route only appears once for a given stop

    std::vector<size_t> firstRouteSegmentOfStop(stop_to_rank.size() + 1);
    std::vector<RAPTOR::RouteSegment> routeSegments;

    size_t current_stop_first_routesegment = 0;

    for (size_t stop_rank = 0; stop_rank < routes_using_a_stop.size(); ++stop_rank) {
        auto& routes_using_this_stop = routes_using_a_stop[stop_rank];

        for (auto & [ route_id, stop_index_in_this_route ] : routes_using_this_stop) {
            auto routeRank = routeToRank.at(route_id);
            routeSegments.emplace_back(RouteId{routeRank}, StopIndex{stop_index_in_this_route});
        }

        firstRouteSegmentOfStop[stop_rank] = current_stop_first_routesegment;
        current_stop_first_routesegment += routes_using_this_stop.size();
    }

    // From now on :
    //      current_stop_first_routesegment = total number of routesgments
    // Setting past-the-end routeSegments :
    auto nb_routes = routes_using_a_stop.size();
    firstRouteSegmentOfStop[nb_routes] = current_stop_first_routesegment;

    return {routeSegments, firstRouteSegmentOfStop};
}
