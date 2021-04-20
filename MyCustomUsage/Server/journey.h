#pragma once

#include "Algorithms/RAPTOR/InitialTransfers.h"
#include "DataStructures/RAPTOR/Data.h"
#include "Helpers/Types.h"
#include "legs.h"

namespace myserver {

// this structure was originally in Algorithms/RAPTOR/ULTRARAPTOR.h
// but we need it to be public to be able to properly split the server code
struct EarliestArrivalLabel {
    EarliestArrivalLabel()
        : arrivalTime(never), parentDepartureTime(never), parent(noVertex), usesRoute(false), routeId(noRouteId) {}
    int arrivalTime;
    int parentDepartureTime;
    Vertex parent;
    bool usesRoute;
    union {
        RouteId routeId;
        Edge transferId;
    };

    std::string as_string() const {
        std::ostringstream oss;
        oss << "[";
        oss << "arrivalTime=" << arrivalTime << "|";
        oss << "parentDepartureTime=" << parentDepartureTime << "|";
        oss << "parent=" << parent << "|";
        oss << "usesRoute=" << usesRoute << "|";
        oss << "routeId=" << routeId << "|";
        oss << "transferId=" << transferId << "|";
        oss << "]";
        return oss.str();
    }
};
using Round = std::vector<EarliestArrivalLabel>;

inline EarliestArrivalLabel get_best_label(int stop, std::vector<Round> const& rounds) {
    for (auto round_ite = rounds.rbegin(); round_ite != rounds.rend(); ++round_ite) {
        auto& label = (*round_ite)[stop];
        if (label.arrivalTime == never)
            continue;
        return label;
    }
    return {};
}

inline std::tuple<::StopId, int, myserver::EarliestArrivalLabel> _find_optimal_last_stop(
    RAPTOR::Data const& data,
    RAPTOR::BucketCHInitialTransfers const& initialTransfers,
    std::vector<Round> const& rounds) {
    // finds the last stop of the optimal journey

    int best_stop = 0;
    int best_distance = INFTY;
    myserver::EarliestArrivalLabel best_label = {};

    // optimal last stop is the one that minimizes EAT(laststop) + distance(laststop -> targetVertex)
    for (int candidate_stop = 0; candidate_stop < data.numberOfStops(); ++candidate_stop) {
        auto from_stop_to_target = initialTransfers.getBackwardDistance(Vertex(candidate_stop));
        auto candidate_label = get_best_label(candidate_stop, rounds);
        if (candidate_label.arrivalTime == never)
            continue;
        auto candidate_distance = from_stop_to_target + candidate_label.arrivalTime;
        if (candidate_distance < best_distance) {
            best_distance = candidate_distance;
            best_label = candidate_label;
            best_stop = candidate_stop;
        }
    }

    // FIXME = handle errors
    return {::StopId{best_stop}, best_distance, best_label};
}

inline std::vector<Leg> build_legs(Vertex source,
                                   Vertex target,
                                   const int requestedDepartureTime,
                                   RAPTOR::Data const& data,
                                   RAPTOR::BucketCHInitialTransfers const& initialTransfers,
                                   std::vector<Round> const& rounds) {
    // journey is rebuilt backward : we recursively get parents, beginning with last_stop

    auto [last_stop, last_walk_distance, last_stop_label] = _find_optimal_last_stop(data, initialTransfers, rounds);

    // for now, we only allow journeys from/to a stop -> targetVertex is necessary a stop
    // (but last_walk_distance is not necessarily 0, if a more optimal stop + final walk exist)
    // if last_walk_distance is INFTY, we couldn't find a suitable last_stop :
    if (last_walk_distance == INFTY) {
        std::cout << "ERROR : last_walk_distance is INFTY, returning empty legs." << std::endl;
        return {};
    }
    assert(last_walk_distance == 0);
    assert(last_stop == target);

    std::vector<Leg> legs;

    std::cout << "About to reconstruct (backward) journey from source=" << source << " to target=" << target
              << " (using last_stop=" << last_stop << ")" << std::endl;
    auto currentStop = Vertex(last_stop);
    auto currentStopLabel = get_best_label(currentStop, rounds);

    // conversion from stop (and its label) to a Leg :
    auto const& raptorData = data;
    auto to_leg = [&raptorData](myserver::EarliestArrivalLabel const& label, int stop) -> Leg {
        bool is_walk = !label.usesRoute;
        std::string departure_id = std::to_string(label.parent);
        std::string arrival_id = std::to_string(stop);
        int departure_time = label.parentDepartureTime;
        int start_time = departure_time;
        int arrival_time = label.arrivalTime;
        std::cout << "\tleg ";
        std::cout << "FROM=" << label.parent << " (" << raptorData.stopData[label.parent] << ", at "
                  << label.parentDepartureTime << ") ";
        std::cout << "TO=" << stop << "(" << raptorData.stopData[stop] << ", at " << label.arrivalTime << ")"
                  << std::endl;
        return {is_walk, departure_id, arrival_id, start_time, departure_time, arrival_time};
    };

    legs.push_back(to_leg(currentStopLabel, currentStop.value()));

    while (currentStopLabel.parent != source && currentStopLabel.parent != currentStop) {
        currentStop = currentStopLabel.parent;
        currentStopLabel = get_best_label(currentStop, rounds);
        legs.push_back(to_leg(currentStopLabel, currentStop.value()));
    }

    // as journey was rebuilt backward, we put it back in proper order :
    std::reverse(legs.begin(), legs.end());

    // it is still unclear if having last_stop != target is a bug or no
    // while this is clarified, we manually add a last walking leg in those situation, to display a proper route in
    // viewer
    if (last_stop != target) {
        auto distance_from_stop_to_target = initialTransfers.getBackwardDistance(Vertex(last_stop));

        auto leg_of_last_stop = *legs.crbegin();
        auto arrival_at_last_stop = leg_of_last_stop.arrival_time;

        bool is_walk = true;
        std::string departure_id = std::to_string(last_stop);
        std::string arrival_id = std::to_string(target);
        int start_time = leg_of_last_stop.arrival_time;
        int departure_time = start_time;
        int arrival_time = start_time + distance_from_stop_to_target;
        std::cout << "\tMANUAL LAST leg ";
        std::cout << "FROM=" << last_stop << " (" << raptorData.stopData[last_stop] << ", at " << start_time << ") ";
        std::cout << "TO=" << target << "(" << raptorData.stopData[target] << ", at " << arrival_time << ")"
                  << std::endl;

        legs.emplace_back(is_walk, departure_id, arrival_id, start_time, departure_time, arrival_time);
    }

    // setting the wait_time of all legs.
    // first leg's wait_time is the difference between the requested departure_time and the leg's departure_time :
    auto& first_leg = legs.front();
    first_leg.start_time = requestedDepartureTime;
    assert(first_leg.start_time <= first_leg.departure_time);

    // for the other leg, the wait_time is the difference between the previous leg's arrival_time and the current leg's
    // departure_time :
    if (legs.size() > 1) {
        for (size_t i_leg = 1; i_leg < legs.size(); ++i_leg) {
            auto const& left_leg = legs[i_leg - 1];
            auto& right_leg = legs[i_leg];
            right_leg.start_time = left_leg.arrival_time;
            assert(right_leg.start_time <= right_leg.departure_time);
        }
    }

    return legs;
}

}  // namespace myserver
