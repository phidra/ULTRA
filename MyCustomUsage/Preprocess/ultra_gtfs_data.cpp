#include <iostream>
#include <numeric>
#include <algorithm>

#include "ultra_gtfs_data.h"
#include "gtfs/gtfs_parsed_data.h"
#include "Preprocess/autodeletefile.h"

using namespace std;

// This code helps to build the RAPTOR binary expected by ULTRA, from a given GTFS folder.

namespace my::preprocess {

static vector<RAPTOR::Route> build_routeData(vector<uwpreprocess::RouteLabel> const& ranked_routes) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(ranked_routes.size());
    // The name of the RAPTOR::route is NOT its rank, but its label, i.e. the concatenation of its stop's ids
    transform(ranked_routes.begin(), ranked_routes.end(), back_inserter(routeData),
              [](auto& routeLabel) { return RAPTOR::Route(routeLabel); });
    return routeData;
}

static vector<RAPTOR::Stop> build_stopData(vector<uwpreprocess::ParsedStop> const& ranked_stops) {
    vector<RAPTOR::Stop> stopData(ranked_stops.size());
    for (size_t rank = 0; rank < ranked_stops.size(); ++rank) {
        uwpreprocess::ParsedStop stop = ranked_stops[rank];
        Geometry::Point location{Construct::LatLongTag{}, stop.latitude, stop.longitude};
        stopData[rank] = RAPTOR::Stop{stop.name, location};
    }
    return stopData;
}

static pair<vector<::StopId>, vector<size_t>> build_stopIdsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<string, size_t> const& stopid_to_rank) {
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    vector<::StopId> stopIds;

    size_t currentRouteFirstStop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        uwpreprocess::RouteLabel routeLabel = routeData[routeRank].name;
        vector<string> stopsOfCurrentRoute = routeLabel.to_stop_ids();
        transform(stopsOfCurrentRoute.cbegin(), stopsOfCurrentRoute.cend(), back_inserter(stopIds),
                  [&stopid_to_rank](string const& stopid) {
                      return ::StopId{static_cast<u_int32_t>(stopid_to_rank.at(stopid))};
                  });

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

static pair<vector<RAPTOR::StopEvent>, vector<size_t>> build_stopEventsRelated(
    vector<RAPTOR::Route> const& routeData,
    map<uwpreprocess::RouteLabel, uwpreprocess::ParsedRoute> const& routes) {
    vector<RAPTOR::StopEvent> allStopEvents;
    vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t currentRouteFirstStopEvent = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        uwpreprocess::RouteLabel routeLabel = routeData[routeRank].name;

        size_t nbStopEventsInThisRoute = 0;

        auto const& route = routes.at(routeLabel);
        for (auto& [tripId, tripEvents] : route.trips) {
            for (auto& [arrTime, depTime] : tripEvents) {
                allStopEvents.emplace_back(arrTime, depTime);
            }
            nbStopEventsInThisRoute += tripEvents.size();
        }

        firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;
        currentRouteFirstStopEvent += nbStopEventsInThisRoute;
    }

    // From now on :
    //      routeRank = number of routes
    //      currentRouteFirstStopEvent = number of stopevents in all the routes
    // Setting past-the-end allStopEvents :
    firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;

    return {allStopEvents, firstStopEventOfRoute};
}

static pair<vector<RAPTOR::RouteSegment>, vector<size_t>> convert_routeSegmentsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<string, size_t> const& stopid_to_rank,
    unordered_map<uwpreprocess::RouteLabel, size_t> const& route_to_rank) {
    // building an intermediate structure that associates a stopRank to all its routes
    // for a given stop, this structures stores some pairs {route + stop index in this route}

    auto nb_stops = stopid_to_rank.size();
    vector<vector<pair<uwpreprocess::RouteLabel, int>>> routesUsingAStop(nb_stops);

    for (auto& route : routeData) {
        uwpreprocess::RouteLabel routeLabel = route.name;
        vector<string> stopsOfThisRoute = routeLabel.to_stop_ids();

        for (size_t stopIndex = 0; stopIndex < stopsOfThisRoute.size(); ++stopIndex) {
            string const& stopid = stopsOfThisRoute[stopIndex];
            size_t stopRank = stopid_to_rank.at(stopid);

            vector<pair<uwpreprocess::RouteLabel, int>>& routesUsingThisStop = routesUsingAStop[stopRank];
            routesUsingThisStop.emplace_back(routeLabel, stopIndex);
        }
    }

    // From now on, for each stop, we know the routes that uses it

    // TODO = check that each route only appears once for a given stop

    vector<size_t> firstRouteSegmentOfStop(stopid_to_rank.size() + 1);
    vector<RAPTOR::RouteSegment> routeSegments;

    size_t currentStopFirstRouteSegment = 0;

    for (size_t stopRank = 0; stopRank < routesUsingAStop.size(); ++stopRank) {
        auto& routesUsingThisStop = routesUsingAStop[stopRank];

        for (auto& [routeId, stopIndexInThisRoute] : routesUsingThisStop) {
            auto routeRank = static_cast<u_int32_t>(route_to_rank.at(routeId));
            routeSegments.emplace_back(::RouteId{routeRank}, StopIndex{static_cast<u_int32_t>(stopIndexInThisRoute)});
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

my::preprocess::UltraGtfsData::UltraGtfsData(uwpreprocess::GtfsParsedData const& gtfs) {
    // use GTFS parsed data to build ULTRA data :
    routeData = build_routeData(gtfs.ranked_routes);
    stopData = build_stopData(gtfs.ranked_stops);
    tie(stopIds, firstStopIdOfRoute) = build_stopIdsRelated(routeData, gtfs.stopid_to_rank);
    tie(stopEvents, firstStopEventOfRoute) = build_stopEventsRelated(routeData, gtfs.routes);
    tie(routeSegments, firstRouteSegmentOfStop) =
        convert_routeSegmentsRelated(routeData, gtfs.stopid_to_rank, gtfs.route_to_rank);

    // STUB : according to some comments in ULTRARAPTOR.h, buffer times have to be implicit :
    implicitDepartureBufferTimes = true;
    implicitArrivalBufferTimes = true;
}

void my::preprocess::UltraGtfsData::dump(string const& filename) const {
    IO::serialize(filename, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds,
                  stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
}

bool my::preprocess::UltraGtfsData::checkSerializationIdempotence(ostream& out) const {
    my::AutoDeleteTempFile tmpfile;

    // serializing in a temporary file :
    dump(tmpfile.file);

    // unserializing, to check that serialization+deserialization is idempotent :
    vector<size_t> freshFirstRouteSegmentOfStop;
    vector<size_t> freshFirstStopIdOfRoute;
    vector<size_t> freshFirstStopEventOfRoute;
    vector<RAPTOR::RouteSegment> freshRouteSegments;
    vector<::StopId> freshStopIds;
    vector<RAPTOR::StopEvent> freshStopEvents;
    vector<RAPTOR::Stop> freshStopData;
    vector<RAPTOR::Route> freshRouteData;
    bool freshImplicitDepartureBufferTimes;
    bool freshImplicitArrivalBufferTimes;

    IO::deserialize(tmpfile.file, freshFirstRouteSegmentOfStop, freshFirstStopIdOfRoute, freshFirstStopEventOfRoute,
                    freshRouteSegments, freshStopIds, freshStopEvents, freshStopData, freshRouteData,
                    freshImplicitDepartureBufferTimes, freshImplicitArrivalBufferTimes);

    bool areFirstRouteSegmentOfStopEqual = firstRouteSegmentOfStop == freshFirstRouteSegmentOfStop;
    bool areFirstStopIdOfRouteEqual = firstStopIdOfRoute == freshFirstStopIdOfRoute;
    bool areFirstStopEventOfRouteEqual = firstStopEventOfRoute == freshFirstStopEventOfRoute;
    bool areRouteSegmentsEqual =
        equal(routeSegments.begin(), routeSegments.end(), freshRouteSegments.begin(),
              [](auto const& x, auto const& y) { return x.routeId == y.routeId && x.stopIndex == y.stopIndex; });
    bool areStopIdsEqual = stopIds == freshStopIds;
    bool areStopEventsEqual =
        equal(stopEvents.begin(), stopEvents.end(), freshStopEvents.begin(), [](auto const& x, auto const& y) {
            return x.arrivalTime == y.arrivalTime && x.departureTime == y.departureTime;
        });
    bool areStopDataEqual =
        equal(stopData.begin(), stopData.end(), freshStopData.begin(), [](auto const& x, auto const& y) {
            return x.name == y.name && x.coordinates == y.coordinates && x.minTransferTime == y.minTransferTime;
        });
    bool areRouteDataEqual = equal(routeData.begin(), routeData.end(), freshRouteData.begin(),
                                   [](auto const& x, auto const& y) { return x.name == y.name && x.type == y.type; });
    bool areImplicitDepartureBufferTimesEqual = implicitDepartureBufferTimes == freshImplicitDepartureBufferTimes;
    bool areImplicitArrivalBufferTimesEqual = implicitArrivalBufferTimes == freshImplicitArrivalBufferTimes;

    out << "DETAILS : is serialization + deserialization idempotent ?" << endl;
    out << boolalpha;
    out << areFirstRouteSegmentOfStopEqual << endl;
    out << areFirstStopIdOfRouteEqual << endl;
    out << areFirstStopEventOfRouteEqual << endl;
    out << areRouteSegmentsEqual << endl;
    out << areStopIdsEqual << endl;
    out << areStopEventsEqual << endl;
    out << areStopDataEqual << endl;
    out << areRouteDataEqual << endl;
    out << areImplicitDepartureBufferTimesEqual << endl;
    out << areImplicitArrivalBufferTimesEqual << endl;

    return (areFirstRouteSegmentOfStopEqual && areFirstStopIdOfRouteEqual && areFirstStopEventOfRouteEqual &&
            areRouteSegmentsEqual && areStopIdsEqual && areStopEventsEqual && areStopDataEqual && areRouteDataEqual &&
            areImplicitDepartureBufferTimesEqual && areImplicitArrivalBufferTimesEqual);
}

void UltraGtfsData::serialize(const string& fileName) const {
    IO::serialize(fileName, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds,
                  stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
}

}  // namespace my::preprocess
