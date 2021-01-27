#pragma once

#include <iostream>
#include <string>
#include <numeric>
#include <unordered_set>

#include "ad/cppgtfs/Parser.h"

namespace my {

using StopSetId = std::string;
using TripId = std::string;
using RouteId = std::string;
using ParsedStopId = std::string;

inline StopSetId build_stopset_id(ad::cppgtfs::gtfs::Trip const& trip) {
    if (trip.getStopTimes().size() < 2) {
        std::ostringstream oss;
        oss << "ERROR : stopset is too small (" << trip.getStopTimes().size() << ") for trip : " << trip.getId();
        throw std::runtime_error(oss.str());
    }

    StopSetId stopset_id{};

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        stopset_id.append(stop.getId());
        stopset_id.append("+");
    }

    // remove final '+' :
    return stopset_id.substr(0, stopset_id.size() - 1);
}

inline std::vector<ParsedStopId> stopset_id_to_stops(StopSetId const& stopset) {
    std::vector<ParsedStopId> stops;
    std::string token;
    std::istringstream iss(stopset);
    while (std::getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

inline RouteId route_id_from_trip_id(ad::cppgtfs::gtfs::Feed const& feed, TripId const& trip_id) {
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)";
        throw std::runtime_error(oss.str());
    }
    auto& trip = *(trip_ptr);
    auto real_route = *(trip.getRoute());
    return real_route.getId();
}

inline std::pair<std::map<StopSetId, std::set<TripId>>, std::unordered_map<RouteId, std::unordered_set<StopSetId>>>
partition_trips_in_stopsets(ad::cppgtfs::gtfs::Feed const& feed) {
    // Cette fonction partitionne les trips selon leur stopset
    // deux trips auront le même stopset s'ils ont exactement les mêmes stops
    //
    // Au sujet des deux sens du terme "route" :
    //  - la littérature scientifique appelle "route" un set de stops
    //    en particulier, si deux trips s'arrêtent aux mêmes stops, ils ont la même route
    //  - le format GTFS impose d'associer un trip à une "route"
    //    mais cette association est arbitraire, et rien n'oblige à suivre la convention "scientifique"
    //    (elle n'est d'ailleurs pas suivie pour le GTFS de Bordeaux)
    //
    // Comme ULTRA nécessite des "routes" au sens scientifique, il est nécessaire de partitionner les trips.

    // associe un stopset (donc une route au sens scientifique) à ses trips
    std::map<StopSetId, std::set<TripId>> stopsetToTrips;
    std::unordered_map<RouteId, std::unordered_set<StopSetId>>
        routesToStopsets;  // à partir d'une route GTFS, retrouve ses routes "scientifiques"

    for (auto const & [ trip_id, trip_ptr ] : feed.getTrips()) {
        auto& trip = *(trip_ptr);
        StopSetId this_stopset_id = build_stopset_id(trip);
        RouteId this_route_id = trip.getRoute()->getId();

        stopsetToTrips[this_stopset_id].emplace(trip_id);
        routesToStopsets[this_route_id].emplace(this_stopset_id);
    }
    return {stopsetToTrips, routesToStopsets};
}

inline std::pair<std::vector<StopSetId>, std::unordered_map<StopSetId, size_t>> rank_routes(
    std::map<StopSetId, std::set<TripId>> const& stopsetToTrips) {
    // à partir des stops partitionnés en stopsets (i.e. en routes au sens scientifique du terme),
    // cette fonction attribue à chaque route un rank donné.

    size_t stopset_counter = 0;
    std::vector<StopSetId> ranked_routes;
    std::unordered_map<StopSetId, size_t> routeidToRank;

    for (auto ite = stopsetToTrips.cbegin(); ite != stopsetToTrips.cend(); ++ite) {
        StopSetId const& routeid = ite->first;
        ranked_routes.push_back(routeid);
        routeidToRank.insert({routeid, stopset_counter++});
    }

    // À ce stade :
    //   - ranked_routes associe un rank à une route (plus précisément, à un StopSetId)
    //   - routeidToRank associe un StopSetId au rank d'une route
    return {ranked_routes, routeidToRank};
}

inline std::pair<std::vector<ParsedStopId>, std::unordered_map<ParsedStopId, size_t>> rank_stops(
    std::map<StopSetId, std::set<TripId>> const& stopsetToTrips) {
    // à partir des stops partitionnés en stopsets (i.e. en routes au sens scientifique du terme),
    // cette fonction attribue à chaque stop un rank donné.

    std::set<ParsedStopId> stops;
    for (auto & [ stopset_id, _ ] : stopsetToTrips) {
        std::vector<ParsedStopId> this_route_stops = my::stopset_id_to_stops(stopset_id);
        stops.insert(this_route_stops.begin(), this_route_stops.end());
    }

    // À ce stade, le set contient tous les stops.

    std::vector<ParsedStopId> ranked_stops(stops.begin(), stops.end());
    std::unordered_map<ParsedStopId, size_t> stopidToRank;
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        ParsedStopId stopid = ranked_stops[rank];
        stopidToRank.insert({stopid, rank});
    }

    // à ce stade :
    //  - ranked_stops permet de retrouver l'id d'un stop à partir de son rank
    //  - stopidToRank permet de retrouver le rank d'un stop à partir de son id
    return {ranked_stops, stopidToRank};
}

// vérifie que tous les trips d'un stopset donné ont bien la même route :
inline void assert_identical_stopset_routes(ad::cppgtfs::gtfs::Feed const& feed,
                                            std::map<StopSetId, std::set<TripId>> const& stopsetToTrips) {
    for (auto[stopset_id, trips] : stopsetToTrips) {
        RouteId reference_route_id = route_id_from_trip_id(feed, *trips.begin());

        auto is_mismatch = [&reference_route_id, &feed](auto trip_id) {
            return route_id_from_trip_id(feed, trip_id) != reference_route_id;
        };

        auto mismatching_trip = find_if(trips.cbegin(), trips.cend(), is_mismatch);
        if (mismatching_trip != trips.cend()) {
            std::cout << "This stopset has more than 1 real route : " << stopset_id << std::endl;
            std::cout << "Reference route_id = " << reference_route_id << std::endl;
            std::cout << "Mismatching trip = " << *mismatching_trip << std::endl;
            std::cout << "Mismatching trip's route id = " << route_id_from_trip_id(feed, *mismatching_trip)
                      << std::endl;
            throw std::runtime_error("failed to assert_identical_stopset_routes");
        }
    }
}

inline ad::cppgtfs::gtfs::Trip const& get_trip(ad::cppgtfs::gtfs::Feed const& feed, TripId const& trip_id) {
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)";
        throw std::runtime_error(oss.str());
    }
    return *trip_ptr;
}

}  // namespace my
