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


#include <cassert>

#include "../../Custom/legs.h"

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


    inline std::tuple<StopId, int, EarliestArrivalLabel> find_optimal_last_stop() {
        // finds the last stop of the optimal journey

        int best_stop = 0;
        int best_distance = INFTY;
        EarliestArrivalLabel best_label = {};

        // optimal last stop is the one that minimizes EAT(laststop) + distance(laststop -> targetVertex)
        for (int candidate_stop = 0; candidate_stop < data.numberOfStops(); ++candidate_stop) {
            auto from_stop_to_target = initialTransfers.getBackwardDistance(Vertex(candidate_stop));
            auto candidate_label = get_best_label(candidate_stop);
            if (candidate_label.arrivalTime == never) continue;
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


    inline std::vector<Leg> build_legs(Vertex source) {
        // journey is rebuilt backward : we recursively get parents, beginning with last_stop

        auto [last_stop, last_walk_distance, last_stop_label] = find_optimal_last_stop();

        // for now, we only allow journeys from/to as top -> targetVertex is necessary a stop, and last_walk_distance is necessary 0 :
        assert(last_walk_distance == 0);
        assert(last_stop == targetStop);

        std::vector<Leg> legs;

        auto currentStop = Vertex(last_stop);
        auto currentStopLabel = get_best_label(currentStop);

        // conversion from stop (and its label) to a Leg :
        auto const& raptorData = this->data;
        auto to_leg = [&raptorData](EarliestArrivalLabel const& label, int stop) -> Leg {
            bool is_walk = !label.usesRoute;
            std::string departure_id = std::to_string(label.parent);
            std::string arrival_id = std::to_string(stop);
            int start_time = label.parentDepartureTime;
            int departure_time = label.parentDepartureTime;
            int arrival_time = label.arrivalTime;
            std::cout << "DEPARTURE = " << label.parent << "   " << raptorData.stopData[label.parent] << std::endl;
            std::cout << "ARRIVAL   = " << stop << "   " << raptorData.stopData[stop] << std::endl;
            return {
                is_walk,
                departure_id,
                arrival_id,
                start_time,
                departure_time,
                arrival_time
            };
        };

        legs.push_back(to_leg(currentStopLabel, currentStop.value()));

        while(currentStopLabel.parent != source && currentStopLabel.parent != currentStop) {
            currentStop = currentStopLabel.parent;
            currentStopLabel = get_best_label(currentStop);
            legs.push_back(to_leg(currentStopLabel, currentStop.value()));
        }

        // as journey was rebuilt backward, we put it back in proper order :
        std::reverse(legs.begin(), legs.end());
        return legs;
    }


    inline std::vector<Leg> run(const Vertex source, const int departureTime, const Vertex target, const size_t maxRounds = 50) noexcept {
        debugger.start();
        debugger.startInitialization();
        clear();
        if (!data.isStop(source) || !data.isStop(target)) {
            std::cout << "For now, we can only handle journeys between stops :" << std::endl;
            std::cout << "Is SOURCE a stop ? " << data.isStop(source) << std::endl;
            std::cout << "Is TARGET a stop ? " << data.isStop(target) << std::endl;
            return {};
        }
        initialize(source, departureTime, target);
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
        auto journey = build_legs(source);
        return journey;
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
