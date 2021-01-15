/**********************************************************************************

 Copyright (c) 2019 Jonas Sauer, Tobias Zündorf

 MIT License

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
 modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************************/

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "InitialTransfers.h"

#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/Container/Set.h"
#include "../../DataStructures/Container/Map.h"

#include "Debugger.h"


#include <sstream>

struct InconsistentLegDurations : public std::exception {
    InconsistentLegDurations(
    int start_time_,
    int departure_time_,
    int arrival_time_,
    int full_duration_,
    int waiting_duration_,
    int traveling_duration_
            ) :
        start_time{start_time_},
        departure_time{departure_time_},
        arrival_time{arrival_time_},
        full_duration{full_duration_},
        waiting_duration{waiting_duration_},
        traveling_duration{traveling_duration_}
    {
        std::ostringstream oss;
        oss << "Inconsistent leg durations/times : [";
        oss << "start_time=" << start_time << "|";
        oss << "departure_time=" << departure_time << "|";
        oss << "arrival_time=" << arrival_time << "|";
        oss << "full_duration=" << full_duration << "|";
        oss << "waiting_duration=" << waiting_duration << "|";
        oss << "traveling_duration=" << traveling_duration;
        oss << "]";
        error_msg = oss.str();
    }

    inline const char* what() const throw() {
        return error_msg.c_str();
    }

    int start_time;
    int departure_time;
    int arrival_time;
    int full_duration;
    int waiting_duration;
    int traveling_duration;
    std::string error_msg;
};


struct Leg {
    Leg(bool is_walk_,
        std::string departure_id_,
        std::string arrival_id_,
        int start_time_,
        int departure_time_,
        int arrival_time_,
        std::vector<std::string> stops_) :
        is_walk{is_walk_},
        departure_id{departure_id_},
        arrival_id{arrival_id_},
        start_time{start_time_},
        departure_time{departure_time_},
        arrival_time{arrival_time_},
        stops{stops_}
    {
    }

    bool is_walk;
    std::string departure_id;

    // leg has several times / durations, because it may include some waiting before the traveling :
    int start_time;  // leg's start_time is either the full journey's departure_time, or the arrival_time of the previous leg
    int departure_time;  // time at which we really begin to travel (= start_time + waiting_duration)
    // for walk, waiting_duration=0, and thus full_duration == traveling_duration
    // for PT, full_duration = waiting_duration + traveling_duration
    std::string arrival_id;
    int arrival_time;
    std::vector<std::string> stops;

    inline int get_full_duration() const { return arrival_time - start_time; }
    inline int get_waiting_duration() const { return departure_time - start_time; }
    inline int get_traveling_duration() const { return arrival_time - departure_time; }

    inline void sanity_check() {
        bool expected = (
            get_full_duration() == get_waiting_duration() + get_traveling_duration() &&
            start_time >= 0 &&
            departure_time >= 0 &&
            arrival_time >= 0 &&
            get_full_duration() >= 0 &&
            get_waiting_duration() >= 0 &&
            get_traveling_duration() >= 0
        );
        if (!expected) {
            throw InconsistentLegDurations(
                start_time,
                departure_time,
                arrival_time,
                get_full_duration(),
                get_waiting_duration(),
                get_traveling_duration()
            );
        }
    }

    inline std::string as_string() const {
        std::ostringstream oss;
        oss << "[" << (is_walk ? "walk" : " tc ") << "]";
        oss << "  FROM=" << departure_id << "  ->  TO=" << arrival_id;
        oss << "  (start at " << start_time << ", begins travel at " << departure_time << "  ....  arrival at " << arrival_time << ")";
        return oss.str();
    }

};


namespace RAPTOR {

template<typename DEBUGGER = NoDebugger>
class ULTRARAPTOR {

public:
    using Debugger = DEBUGGER;
    using Type = ULTRARAPTOR<Debugger>;

private:
    struct EarliestArrivalLabel {
        EarliestArrivalLabel() : arrivalTime(never), parentDepartureTime(never), parent(noVertex), usesRoute(false), routeId(noRouteId) {}
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

public:
    template<typename ATTRIBUTE>
    ULTRARAPTOR(const Data& data, const CHGraph& forwardGraph, const CHGraph& backwardGraph, const ATTRIBUTE weight, const Debugger& debuggerTemplate = Debugger()) :
        data(data),
        initialTransfers(forwardGraph, backwardGraph, data.numberOfStops(), weight),
        earliestArrival(data.numberOfStops() + 1),
        stopsUpdatedByRoute(data.numberOfStops() + 1),
        stopsUpdatedByTransfer(data.numberOfStops() + 1),
        routesServingUpdatedStops(data.numberOfRoutes()),
        sourceVertex(noVertex),
        targetVertex(noVertex),
        targetStop(noStop),
        debugger(debuggerTemplate) {
        AssertMsg(data.hasImplicitBufferTimes(), "Departure buffer times have to be implicit!");
        debugger.initialize(data);
    }

    ULTRARAPTOR(const Data& data, const CH::CH& chData, const Debugger& debuggerTemplate = Debugger()) :
        ULTRARAPTOR(data, chData.forward, chData.backward, Weight, debuggerTemplate) {
    }

    inline EarliestArrivalLabel get_best_label(int stop) {
        for (auto round_ite = rounds.rbegin(); round_ite != rounds.rend(); ++round_ite) {
            auto& label = (*round_ite)[stop];
            if (label.arrivalTime == never) continue;
            return label;
        }
        return {};
    }

    inline void display_best_label(int stop) {
        EarliestArrivalLabel const & label = get_best_label(stop);
        std::cout << "LABEL OF STOP : " << stop << "   (" << data.stopData[stop] << ")" << std::endl;
        std::cout << "\t arrivalTime = " << label.arrivalTime << std::endl;
        std::cout << "\t parentDepartureTime = " << label.parentDepartureTime << std::endl;
        std::cout << "\t TEMPS DE TRAJET = " << (label.arrivalTime - label.parentDepartureTime) << std::endl;
        std::cout << "\t parent = " << label.parent;
        std::cout.flush();
        if (label.parent.isValid() && data.isStop(label.parent)) {
            std::cout << "   (" << data.stopData[label.parent] << ")" << std::endl;
        }
        else {
            std::cout << "   (INVALID OR NOT A STOP)" << std::endl;
        }
        std::cout << "\t usesRoute = " << label.usesRoute << std::endl;
        if (label.usesRoute) {
            std::cout << "\t UNION> routeId = " << label.routeId << std::endl;
            std::cout << " ROUTE = " << data.routeData[label.routeId] << std::endl;
        }
        else {
            std::cout << "\t UNION> transferId = " << label.transferId << std::endl;
        }
        std::cout << std::endl;
    }


    inline std::vector<Leg> run(const Vertex source, const int departureTime, const Vertex target, const size_t maxRounds = 50) noexcept {
        debugger.start();
        debugger.startInitialization();
        clear();
        if (!data.isStop(source) || !data.isStop(target)) {
            std::cout << "FOR NOW, we can only handle stop source/target" << std::endl;
            std::cout << "SOURCE IS STOP ? " << data.isStop(source) << std::endl;
            std::cout << "TARGET IS STOP ? " << data.isStop(target) << std::endl;
            return {};
        }


        initialize(source, departureTime, target);
        std::cout << "nostop is = " << noStop << std::endl;
        std::cout << "targetStop is stop ? " << data.isStop(targetStop) << std::endl;
        std::cout << "isStop(25427) ? " << data.isStop(Vertex(25427)) << std::endl;
        std::cout << "isStop(25426) ? " << data.isStop(Vertex(25426)) << std::endl;
        std::cout << "isStop(25425) ? " << data.isStop(Vertex(25425)) << std::endl;
        std::cout << "isStop(25424) ? " << data.isStop(Vertex(25424)) << std::endl;
        std::cout << "Target stop is = " << targetStop << std::endl;
        std::cout << "Target stop is = " << targetStop << std::endl;
        std::cout << "Target stop is = " << targetStop << std::endl;
        debugger.doneInitialization();
        relaxInitialTransfers(departureTime);
        for (size_t i = 0; i < maxRounds; i++) {
            debugger.newRound();
            startNewRound();
            collectRoutesServingUpdatedStops();
            scanRoutes();
            if (stopsUpdatedByRoute.empty()) break;
            relaxIntermediateTransfers();
        }

        debugger.done();


        int best_stop = 0;
        int min_distance = INFTY;
        EarliestArrivalLabel min_label = {};
        for (int candidate_stop = 0; candidate_stop < data.numberOfStops(); ++candidate_stop) {
            auto from_stop_to_target = initialTransfers.getBackwardDistance(Vertex(candidate_stop));
            auto best_label = get_best_label(candidate_stop);
            if (best_label.arrivalTime == never) continue;
            auto candidate_distance = from_stop_to_target + best_label.arrivalTime;
            if (candidate_distance < min_distance) {
                min_distance = candidate_distance;
                min_label = best_label;
                best_stop = candidate_stop;
            }
        }

        std::cout << "Le meilleur stop pour la target est : " << best_stop << std::endl;


        Leg mysuperleg(
            false, // is_walk
            "my super departure",  // departure_id
            "my super arrival",  // arrival_id
            42,  // start_time
            42,  // departure_time (for simplicity, start_time == departure_time)
            52,  // arrival_time
            {"coucou", "pouet"} // stops
        );

        std::cout << mysuperleg.as_string() << std::endl;


        /* EarliestArrivalLabel() : arrivalTime(never), parentDepartureTime(never), parent(noVertex), usesRoute(false), routeId(noRouteId) {} */
        /* int arrivalTime; */
        /* int parentDepartureTime; */
        /* Vertex parent; */
        /* bool usesRoute; */
        /* union { */
        /*     RouteId routeId; */
        /*     Edge transferId; */
        /* }; */


        std::vector<Leg> legs;

        auto currentStop = Vertex(best_stop);

        auto currentStopLabel = get_best_label(currentStop);

        auto to_leg = [](EarliestArrivalLabel const& label, int stop) -> Leg {
            bool is_walk = !label.usesRoute;
            std::string departure_id = std::to_string(label.parent);
            std::string arrival_id = std::to_string(stop);
            int start_time = label.parentDepartureTime;
            int departure_time = label.parentDepartureTime;
            int arrival_time = label.arrivalTime;

            // pour le moment, je ne mets que les ids source et target
            std::vector<std::string> stops{departure_id, arrival_id};
            return {
                is_walk,
                departure_id,
                arrival_id,
                start_time,
                departure_time,
                arrival_time,
                stops
            };
        };

        legs.push_back(to_leg(currentStopLabel, currentStop.value()));

        while(currentStopLabel.parent != source && currentStopLabel.parent != currentStop) {
            display_best_label(currentStop);
            currentStop = currentStopLabel.parent;
            currentStopLabel = get_best_label(currentStop);
            legs.push_back(to_leg(currentStopLabel, currentStop.value()));
        }
        display_best_label(currentStop);
        std::cout << "FIIIIIIIIIIIIIIIIIIIIN" << std::endl;

        std::reverse(legs.begin(), legs.end());
        return legs;
    }

    inline const Debugger& getDebugger() const noexcept {
        return debugger;
    }

private:
    template<bool RESET_CAPACITIES = false>
    inline void clear() noexcept {
        stopsUpdatedByRoute.clear();
        stopsUpdatedByTransfer.clear();
        routesServingUpdatedStops.clear();
        targetStop = StopId(data.numberOfStops());
        if constexpr (RESET_CAPACITIES) {
            std::vector<Round>().swap(rounds);
            std::vector<int>(earliestArrival.size(), never).swap(earliestArrival);
        } else {
            rounds.clear();
            Vector::fill(earliestArrival, never);
        }
    }

    inline void reset() noexcept {
        clear<true>();
    }

    inline void initialize(const Vertex source, const int departureTime, const Vertex target) noexcept {
        sourceVertex = source;
        targetVertex = target;
        if (data.isStop(target)) {
            targetStop = StopId(target);
        }
        startNewRound();
        if (data.isStop(source)) {
            debugger.updateStopByRoute(StopId(source), departureTime);
            arrivalByRoute(StopId(source), departureTime);
            currentRound()[source].parent = source;
            currentRound()[source].parentDepartureTime = departureTime;
            currentRound()[source].usesRoute = false;
            stopsUpdatedByTransfer.insert(StopId(source));
        }
    }

    inline void collectRoutesServingUpdatedStops() noexcept {
        debugger.startCollectRoutes();
        for (const StopId stop : stopsUpdatedByTransfer) {
            AssertMsg(data.isStop(stop), "Stop " << stop << " is out of range!");
            const int arrivalTime = previousRound()[stop].arrivalTime;
            AssertMsg(arrivalTime < never, "Updated stop has arrival time = never!");
            for (const RouteSegment& route : data.routesContainingStop(stop)) {
                AssertMsg(data.isRoute(route.routeId), "Route " << route.routeId << " is out of range!");
                AssertMsg(data.stopIds[data.firstStopIdOfRoute[route.routeId] + route.stopIndex] == stop, "RAPTOR data contains invalid route segments!");
                if (route.stopIndex + 1 == data.numberOfStopsInRoute(route.routeId)) continue;
                if (data.lastTripOfRoute(route.routeId)[route.stopIndex].departureTime < arrivalTime) continue;
                if (routesServingUpdatedStops.contains(route.routeId)) {
                    routesServingUpdatedStops[route.routeId] = std::min(routesServingUpdatedStops[route.routeId], route.stopIndex);
                } else {
                    routesServingUpdatedStops.insert(route.routeId, route.stopIndex);
                }
            }
        }
        debugger.stopCollectRoutes();
    }

    inline void scanRoutes() noexcept {
        debugger.startScanRoutes();
        stopsUpdatedByRoute.clear();
        for (const RouteId route : routesServingUpdatedStops.getKeys()) {
            debugger.scanRoute(route);
            StopIndex stopIndex = routesServingUpdatedStops[route];
            const size_t tripSize = data.numberOfStopsInRoute(route);
            AssertMsg(stopIndex < tripSize - 1, "Cannot scan a route starting at/after the last stop (Route: " << route << ", StopIndex: " << stopIndex << ", TripSize: " << tripSize << ")!");

            const StopId* stops = data.stopArrayOfRoute(route);
            const StopEvent* trip = data.lastTripOfRoute(route);
            StopId stop = stops[stopIndex];
            AssertMsg(trip[stopIndex].departureTime >= previousRound()[stop].arrivalTime, "Cannot scan a route after the last trip has departed (Route: " << route << ", Stop: " << stop << ", StopIndex: " << stopIndex << ", Time: " << previousRound()[stop].arrivalTime << ", LastDeparture: " << trip[stopIndex].departureTime << ")!");

            StopIndex parentIndex = stopIndex;
            const StopEvent* firstTrip = data.firstTripOfRoute(route);
            while (stopIndex < tripSize - 1) {
                while ((trip > firstTrip) && ((trip - tripSize + stopIndex)->departureTime >= previousRound()[stop].arrivalTime)) {
                    trip -= tripSize;
                    parentIndex = stopIndex;
                }
                stopIndex++;
                stop = stops[stopIndex];
                debugger.scanRouteSegment(data.getRouteSegmentNum(route, stopIndex));
                if (arrivalByRoute(stop, trip[stopIndex].arrivalTime)) {
                    EarliestArrivalLabel& label = currentRound()[stop];
                    label.parent = stops[parentIndex];
                    label.parentDepartureTime = trip[parentIndex].departureTime;
                    label.usesRoute = true;
                    label.routeId = route;
                }
            }
        }
        debugger.stopScanRoutes();
    }

    inline void relaxInitialTransfers(const int sourceDepartureTime) noexcept {
        debugger.startRelaxTransfers();
        initialTransfers.run(sourceVertex, targetVertex);
        debugger.directWalking(initialTransfers.getDistance());
        for (const Vertex stop : initialTransfers.getForwardPOIs()) {
            if (stop == targetStop) continue;
            AssertMsg(data.isStop(stop), "Reached POI " << stop << " is not a stop!");
            AssertMsg(initialTransfers.getForwardDistance(stop) != INFTY, "Vertex " << stop << " was not reached!");
            const int arrivalTime = sourceDepartureTime + initialTransfers.getForwardDistance(stop);
            if (arrivalByTransfer(StopId(stop), arrivalTime)) {
                debugger.updateStopByTransfer(StopId(stop), arrivalTime);
                EarliestArrivalLabel& label = currentRound()[stop];
                label.parent = sourceVertex;
                label.parentDepartureTime = sourceDepartureTime;
                label.usesRoute = false;
                label.transferId = noEdge;
            }
        }
        if (initialTransfers.getDistance() != INFTY) {
            const int arrivalTime = sourceDepartureTime + initialTransfers.getDistance();
            if (arrivalByTransfer(targetStop, arrivalTime)) {
                debugger.updateStopByTransfer(targetStop, arrivalTime);
                EarliestArrivalLabel& label = currentRound()[targetStop];
                label.parent = sourceVertex;
                label.parentDepartureTime = sourceDepartureTime;
                label.usesRoute = false;
                label.transferId = noEdge;
            }
        }
        debugger.stopRelaxTransfers();
    }

    inline void relaxIntermediateTransfers() noexcept {
        debugger.startRelaxTransfers();
        stopsUpdatedByTransfer.clear();
        routesServingUpdatedStops.clear();
        for (const StopId stop : stopsUpdatedByRoute) {
            const int earliestArrivalTime = currentRound()[stop].arrivalTime;
            for (const Edge edge : data.transferGraph.edgesFrom(stop)) {
                const StopId toStop = StopId(data.transferGraph.get(ToVertex, edge));
                if (toStop == targetStop) continue;
                debugger.relaxEdge(edge);
                const int arrivalTime = earliestArrivalTime + data.transferGraph.get(TravelTime, edge);
                AssertMsg(data.isStop(data.transferGraph.get(ToVertex, edge)), "Graph contains edges to non stop vertices!");
                if (arrivalByTransfer(toStop, arrivalTime)) {
                    debugger.updateStopByTransfer(toStop, arrivalTime);
                    EarliestArrivalLabel& label = currentRound()[toStop];
                    label.parent = stop;
                    label.parentDepartureTime = earliestArrivalTime;
                    label.usesRoute = false;
                    label.transferId = edge;
                }
            }
            if (initialTransfers.getBackwardDistance(stop) != INFTY) {
                const int arrivalTime = earliestArrivalTime + initialTransfers.getBackwardDistance(stop);
                if (arrivalByTransfer(targetStop, arrivalTime)) {
                    debugger.updateStopByTransfer(targetStop, arrivalTime);
                    EarliestArrivalLabel& label = currentRound()[targetStop];
                    label.parent = stop;
                    label.parentDepartureTime = earliestArrivalTime;
                    label.usesRoute = false;
                    label.transferId = noEdge;
                }
            }
            stopsUpdatedByTransfer.insert(stop);
            debugger.settleVertex(stop);
        }
        debugger.stopRelaxTransfers();
    }

    inline Round& currentRound() noexcept {
        AssertMsg(!rounds.empty(), "Cannot return current round, because no round exists!");
        return rounds.back();
    }

    inline Round& previousRound() noexcept {
        AssertMsg(rounds.size() >= 2, "Cannot return previous round, because less than two rounds exist!");
        return rounds[rounds.size() - 2];
    }

    inline void startNewRound() noexcept {
        rounds.emplace_back(data.numberOfStops() + 1);
    }

    inline bool arrivalByRoute(const StopId stop, const int time) noexcept {
        AssertMsg(data.isStop(stop), "Stop " << stop << " is out of range!");
        if (earliestArrival[targetStop] <= time) return false;
        if (earliestArrival[stop] <= time) return false;
        debugger.updateStopByRoute(stop, time);
        currentRound()[stop].arrivalTime = time;
        earliestArrival[stop] = time;
        stopsUpdatedByRoute.insert(stop);
        return true;
    }

    inline bool arrivalByTransfer(const StopId stop, const int time) noexcept {
        AssertMsg(data.isStop(stop) || stop == targetStop, "Stop " << stop << " is out of range!");
        if (earliestArrival[targetStop] <= time) return false;
        if (earliestArrival[stop] <= time) return false;
        currentRound()[stop].arrivalTime = time;
        earliestArrival[stop] = time;
        if (data.isStop(stop)) stopsUpdatedByTransfer.insert(stop);
        return true;
    }

private:
    const Data& data;

    BucketCHInitialTransfers initialTransfers;

    std::vector<Round> rounds;

    std::vector<int> earliestArrival;

    IndexedSet<false, StopId> stopsUpdatedByRoute;
    IndexedSet<false, StopId> stopsUpdatedByTransfer;
    IndexedMap<StopIndex, false, RouteId> routesServingUpdatedStops;

    Vertex sourceVertex;
    Vertex targetVertex;
    StopId targetStop;

    Debugger debugger;

};

}
