#include <iostream>
#include <string>
#include <numeric>
#include <unordered_set>

#include "ad/cppgtfs/Parser.h"

namespace my {

using StopSetId = std::string;
using TripId = std::string;
using RouteId = std::string;
using StopId = std::string;

StopSetId build_stopset_id(ad::cppgtfs::gtfs::Trip const& trip) {
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

std::vector<int> stopset_id_to_stops(StopSetId const& stopset) {
    std::vector<int> stops;
    std::string token;
    std::istringstream iss(stopset);
    while (std::getline(iss, token, '+')) {
        stops.push_back(stoi(token));
    }
    return stops;
}

RouteId route_id_from_trip_id(ad::cppgtfs::gtfs::Feed const& feed, TripId const& trip_id) {
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

std::pair<std::map<StopSetId, std::set<TripId>>, std::unordered_map<RouteId, std::unordered_set<StopSetId>>>
partition_trips_in_stopsets(ad::cppgtfs::gtfs::Feed const& feed) {
    // cette fonction partitionne les trips selon leur stopset
    // deux trips auront le même stopset s'ils ont exactement les mêmes stops
    // dans la littérature scientifique, ce stopset est appelé "route"
    // mais dans le format GTFS, rien n'impose aux trips d'une même route d'avoir les mêmes stops
    // (ça n'est d'ailleur pas le cas pour le GTFS de Bordeaux)

    std::map<StopSetId, std::set<TripId>> stopsetToTrips;
    std::unordered_map<RouteId, std::unordered_set<StopSetId>> routesToStopsets;

    for (auto const & [ trip_id, trip_ptr ] : feed.getTrips()) {
        auto& trip = *(trip_ptr);
        StopSetId this_stopset_id = build_stopset_id(trip);
        RouteId this_route_id = trip.getRoute()->getId();

        stopsetToTrips[this_stopset_id].emplace(trip_id);
        routesToStopsets[this_route_id].emplace(this_stopset_id);
    }

    return {stopsetToTrips, routesToStopsets};
}

// vérifie que tous les trips d'un stopset donné ont bien la même route :
void assert_identical_stopset_routes(ad::cppgtfs::gtfs::Feed const& feed,
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

ad::cppgtfs::gtfs::Trip const& get_trip(ad::cppgtfs::gtfs::Feed const& feed, TripId const& trip_id) {
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        std::ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << trip_id << "' (trip_ptr is 0)";
        throw std::runtime_error(oss.str());
    }
    return *trip_ptr;
}

}  // namespace my
