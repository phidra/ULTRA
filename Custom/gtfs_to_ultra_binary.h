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

inline ad::cppgtfs::gtfs::Trip const& _get_trip(ad::cppgtfs::gtfs::Feed const& feed, TripID const& trip_id) {
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)";
        throw std::runtime_error(oss.str());
    }
    return *trip_ptr;
}

inline std::vector<RAPTOR::Route> build_routeData(std::map<my::RouteID, std::set<my::TripID>> const& route_to_trips) {
    std::vector<RAPTOR::Route> routeData;
    routeData.reserve(route_to_trips.size());
    // note : The route name is its id (and not its rank)
    transform(route_to_trips.begin(), route_to_trips.end(), back_inserter(routeData),
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

/* inline std::pair<std::vector<StopId>, std::vector<size_t>> convert_stopIdsRelated( */
/*     std::vector<RAPTOR::Route> const& routeData, */
/*     std::unordered_map<my::StopID, size_t> const& stopidToRank) { */
/*     std::vector<StopId> stopIds; */
/*     std::vector<size_t> firstStopIdOfRoute(routeData.size() + 1); */

/*     size_t current_route_first_stop = 0; */
/*     int route_index = 0; */
/*     for (auto& route : routeData) { */
/*         my::RouteID routeName = route.name;  // a (scientific) route's name is its stopset */
/*         std::vector<my::StopID> stops_of_this_route = my::stopset_id_to_stops(routeName); */

/*         firstStopIdOfRoute[route_index++] = current_route_first_stop; */
/*         transform(stops_of_this_route.cbegin(), stops_of_this_route.cend(), back_inserter(stopIds), */
/*                   [&stopidToRank](my::StopID const& stopid) { return StopId{stopidToRank.at(stopid)}; }); */
/*         current_route_first_stop += stops_of_this_route.size(); */
/*     } */

/*     // À ce stade : */
/*     //      route_index = nombre de routes */
/*     //      current_route_first_stop = nombre de stops */
/*     // On sette l'index "past-the-end" de stopIds : */
/*     firstStopIdOfRoute[route_index] = current_route_first_stop; */

/*     return {stopIds, firstStopIdOfRoute}; */
/* } */

/* inline std::pair<std::vector<RAPTOR::StopEvent>, std::vector<size_t>> convert_stopEventsRelated( */
/*     ad::cppgtfs::gtfs::Feed const& feed, */
/*     std::vector<RAPTOR::Route> const& routeData, */
/*     std::map<my::RouteID, std::set<my::TripID>> const& route_to_trips) { */
/*     std::vector<RAPTOR::StopEvent> stopEvents; */
/*     std::vector<size_t> firstStopEventOfRoute(routeData.size() + 1); */

/*     size_t current_route_first_stopevent = 0; */
/*     int route_index = 0; */

/*     for (auto& route : routeData) { */
/*         my::RouteID routeName = route.name;  // a "route" name is its stopset */

/*         int nb_stopevents_in_this_route = 0; */

/*         // chaque route (=stopset) est associé à plusieurs trips : */
/*         auto const& trips = route_to_trips.at(routeName); */
/*         for (auto& trip_id : trips) { */
/*             // récupération du trip courant : */
/*             auto& trip = my::_get_trip(feed, trip_id); */

/*             // chaque trip a un range de stopEvents */
/*             // un RAPTOR::StopEvent = {departureTime,arrivalTime} */
/*             // un cppgtfs::StopTime = {_at,_dt} */
/*             for (auto const& stoptime : trip.getStopTimes()) { */
/*                 int departure_time = stoptime.getDepartureTime().seconds(); */
/*                 int arrival_time = stoptime.getArrivalTime().seconds(); */
/*                 stopEvents.emplace_back(arrival_time, departure_time); */
/*             } */

/*             int nb_stopevents_in_this_trip = trip.getStopTimes().size(); */
/*             nb_stopevents_in_this_route += nb_stopevents_in_this_trip; */
/*         } */

/*         firstStopEventOfRoute[route_index++] = current_route_first_stopevent; */
/*         current_route_first_stopevent += nb_stopevents_in_this_route; */
/*     } */

/*     // À ce stade : */
/*     //      route_index = nombre de routes */
/*     //      current_route_first_stopevent = nombre de stopevents */
/*     // On sette l'index "past-the-end" de stopIds : */
/*     firstStopEventOfRoute[route_index] = current_route_first_stopevent; */

/*     return {stopEvents, firstStopEventOfRoute}; */
/* } */

/* inline std::pair<std::vector<RAPTOR::RouteSegment>, std::vector<size_t>> convert_routeSegmentsRelated( */
/*     std::vector<RAPTOR::Route> const& routeData,  // FIXME : on ne devrait pas dépendre de routeData */
/*     std::unordered_map<my::StopID, size_t> const& stopidToRank, */
/*     std::map<my::RouteID, std::set<my::TripID>> const& route_to_trips) { */
/*     // build d'une structure intermédiaire associant un stop (via son rank) à la liste de ses routes */
/*     // pour un stop donné, elle contient à la fois la route concernée, et l'index du stop dans la route */
/*     std::vector<std::vector<std::pair<my::RouteID, int>>> routesOfAStop(stopidToRank.size()); */

/*     for (auto& route : routeData) { */
/*         my::RouteID routeName = route.name;  // a (scientific) route's name is its stopset */
/*         std::vector<my::StopID> stops_of_this_route = my::stopset_id_to_stops(routeName); */

/*         for (size_t stop_index = 0; stop_index < stops_of_this_route.size(); ++stop_index) { */
/*             my::StopID const& this_stop_id = stops_of_this_route[stop_index]; */
/*             size_t this_stop_rank = stopidToRank.at(this_stop_id); */

/*             std::vector<std::pair<my::RouteID, int>>& routes_of_this_stop = routesOfAStop[this_stop_rank]; */
/*             routes_of_this_stop.emplace_back(routeName, stop_index); */
/*         } */
/*     } */

/*     // À ce stade, on connaît pour chaque stop la liste des routes qui l'utilisent */

/*     // TODO = vérifier que chaque route n'apparaît qu'une fois pour un stop donné */

/*     auto[ranked_routes, routeidToRank] = my::rank_routes(route_to_trips); */

/*     std::vector<size_t> firstRouteSegmentOfStop(stopidToRank.size() + 1); */
/*     std::vector<RAPTOR::RouteSegment> routeSegments; */

/*     size_t current_stop_first_routesegment = 0; */

/*     for (size_t stop_rank = 0; stop_rank < routesOfAStop.size(); ++stop_rank) { */
/*         auto& routes_of_this_stop = routesOfAStop[stop_rank]; */

/*         for (auto & [ route_stopset_id, stop_index_in_this_route ] : routes_of_this_stop) { */
/*             auto route_rank = routeidToRank.at(route_stopset_id); */
/*             routeSegments.emplace_back(RouteId{route_rank}, StopIndex{stop_index_in_this_route}); */
/*         } */

/*         firstRouteSegmentOfStop[stop_rank] = current_stop_first_routesegment; */
/*         current_stop_first_routesegment += routes_of_this_stop.size(); */
/*     } */

/*     // On sette l'index "past-the-end" de routeSegments : */
/*     firstRouteSegmentOfStop[routesOfAStop.size()] = current_stop_first_routesegment; */

/*     return {routeSegments, firstRouteSegmentOfStop}; */
/* } */
