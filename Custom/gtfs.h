#include <iostream>
#include <string>
#include <numeric>
#include <unordered_set>

#include "ad/cppgtfs/Parser.h"

namespace my {

using StopSetId = std::string;
using TripId = std::string;

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

std::string route_id_from_trip_id(ad::cppgtfs::gtfs::Feed const& feed, TripId const& trip_id) {
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

std::pair<std::unordered_map<StopSetId, std::unordered_set<TripId>>, std::unordered_map<TripId, StopSetId>>
partition_trips_in_stopsets(ad::cppgtfs::gtfs::Feed const& feed) {
    // cette fonction partitionne les trips selon leur stopset
    // deux trips auront le même stopset s'ils ont exactement les mêmes stops
    // dans la littérature scientifique, ce stopset est appelé "route"
    // mais dans le format GTFS, rien n'impose aux trips d'une même route d'avoir les mêmes stops
    // (ça n'est d'ailleur pas le cas pour le GTFS de Bordeaux)

    // ce que je veux faire :
    //      itérer sur les trips
    //      éventuellement, créer une clé à partir d'un set de stops (e.g. la concaténation des stop_ids)
    //      regrouper les trips par cette clé (et ne pas oublier d'associer également la route originale)
    //      in fine, lister :
    //          - vérifier que toutes les routes "réelles" au sein d'une même route virtuelle sont identiques
    //          - lister le nombre de routes virtuelles (au total, et par route réelle)
    //          - lister le nombre de trips par route virtuelle

    // stocke pour chaque set de stops les trips associés :
    std::unordered_map<StopSetId, std::unordered_set<TripId>> stopsetToTrips;
    std::unordered_map<TripId, StopSetId> tripsToStopset;

    for (auto const & [ trip_id, trip_ptr ] : feed.getTrips()) {
        auto& trip = *(trip_ptr);
        StopSetId this_stopset_id = build_stopset_id(trip);

        // si le stopset n'a encore jamais été rencontré, on créé un nouveau set :
        if (stopsetToTrips.find(this_stopset_id) == stopsetToTrips.end()) {
            stopsetToTrips[this_stopset_id] = {trip_id};
        }
        // sinon, on ajoute au set le trip :
        else {
            auto& stopset_trips = stopsetToTrips[this_stopset_id];
            stopset_trips.emplace(trip_id);
        }

        // dans tous les cas, on associe ce trip à son stopset :
        tripsToStopset[trip_id] = this_stopset_id;
    }

    return {stopsetToTrips, tripsToStopset};
}

// vérifie que tous les trips d'un stopset donné ont bien la même route :
void assert_identical_stopset_routes(ad::cppgtfs::gtfs::Feed const& feed,
                                     std::unordered_map<StopSetId, std::unordered_set<TripId>> const& stopsetToTrips) {
    for (auto[stopset_id, trips] : stopsetToTrips) {
        std::string reference_route_id = route_id_from_trip_id(feed, *trips.begin());

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
}
