#pragma once

#include "../DataStructures/RAPTOR/Data.h"
#include "../Helpers/Types.h"

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

inline std::tuple<StopId, int, myserver::EarliestArrivalLabel> _find_optimal_last_stop(
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
    return {StopId(best_stop), best_distance, best_label};
}

inline std::vector<Leg> build_legs(Vertex source,
                                   RAPTOR::Data const& data,
                                   RAPTOR::BucketCHInitialTransfers const& initialTransfers,
                                   std::vector<Round> const& rounds) {
    // journey is rebuilt backward : we recursively get parents, beginning with last_stop

    auto[last_stop, last_walk_distance, last_stop_label] = _find_optimal_last_stop(data, initialTransfers, rounds);

    // for now, we only allow journeys from/to as top -> targetVertex is necessary a stop, and last_walk_distance is
    // necessary 0 :
    assert(last_walk_distance == 0);
    assert(last_stop == targetStop);

    std::vector<Leg> legs;

    auto currentStop = Vertex(last_stop);
    auto currentStopLabel = get_best_label(currentStop, rounds);

    // conversion from stop (and its label) to a Leg :
    auto const& raptorData = data;
    auto to_leg = [&raptorData](myserver::EarliestArrivalLabel const& label, int stop) -> Leg {
        bool is_walk = !label.usesRoute;
        std::string departure_id = std::to_string(label.parent);
        std::string arrival_id = std::to_string(stop);
        int start_time = label.parentDepartureTime;
        int departure_time = label.parentDepartureTime;
        int arrival_time = label.arrivalTime;
        std::cout << "DEPARTURE = " << label.parent << "   " << raptorData.stopData[label.parent] << std::endl;
        std::cout << "ARRIVAL   = " << stop << "   " << raptorData.stopData[stop] << std::endl;
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
    return legs;
}

}  // namespace myserver
