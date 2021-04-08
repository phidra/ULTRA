#include <iostream>
#include <numeric>
#include <algorithm>

#include "ultra_gtfs_data.h"
#include "gtfs_parsed_data.h"
#include "Common/autodeletefile.h"

using namespace std;

// This code helps to build the RAPTOR binary expected by ULTRA, from a given GTFS feed.

namespace my::preprocess {


static ad::cppgtfs::gtfs::Trip const& getTrip(ad::cppgtfs::gtfs::Feed const& feed, my::preprocess::TripLabel const& tripId) {
    auto tripPtr = feed.getTrips().get(tripId);
    if (tripPtr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get trip with id '" << tripId << "' (tripPtr is 0)";
        throw runtime_error(oss.str());
    }
    return *tripPtr;
}

static vector<RAPTOR::Route> build_routeData(vector<RouteLabel> const& rankedRoutes) {
    vector<RAPTOR::Route> routeData;
    routeData.reserve(rankedRoutes.size());
    // The name of the RAPTOR::route is NOT its rank, but its label, i.e. the concatenation of its stop's ids
    transform(rankedRoutes.begin(), rankedRoutes.end(), back_inserter(routeData),
              [](auto& routeLabel) { return RAPTOR::Route(routeLabel); });
    return routeData;
}

static vector<RAPTOR::Stop> build_stopData(vector<my::preprocess::ParsedStop> const& rankedStops) {
    vector<RAPTOR::Stop> stopData(rankedStops.size());
    for (size_t rank = 0; rank < rankedStops.size(); ++rank) {
        my::preprocess::ParsedStop stop = rankedStops[rank];
        Geometry::Point location{Construct::LatLongTag{}, stop.latitude, stop.longitude};
        stopData[rank] = RAPTOR::Stop{stop.name, location};
    }
    return stopData;
}

static pair<vector<::StopId>, vector<size_t>> build_stopIdsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<string, size_t> const& stopidToRank) {
    vector<size_t> firstStopIdOfRoute(routeData.size() + 1);
    vector<::StopId> stopIds;

    size_t currentRouteFirstStop = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::preprocess::RouteLabel routeLabel = routeData[routeRank].name;
        vector<string> stopsOfCurrentRoute = routeLabel.toStopIds();
        transform(stopsOfCurrentRoute.cbegin(), stopsOfCurrentRoute.cend(), back_inserter(stopIds),
                  [&stopidToRank](string const& stopid) { return ::StopId{static_cast<u_int32_t>(stopidToRank.at(stopid))}; });

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
    map<my::preprocess::RouteLabel, set<my::preprocess::OrderableTripLabel>> const& routeToTrips,
    ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::StopEvent> stopEvents;
    vector<size_t> firstStopEventOfRoute(routeData.size() + 1);

    size_t currentRouteFirstStopEvent = 0;
    size_t routeRank;
    for (routeRank = 0; routeRank < routeData.size(); ++routeRank) {
        my::preprocess::RouteLabel routeId = routeData[routeRank].name;

        size_t nbStopEventsInThisRoute = 0;

        // each route "contains" (=is associated with) several trips :
        auto const& tripsOfCurrentRoute = routeToTrips.at(routeId);
        for (auto& [_, tripId] : tripsOfCurrentRoute) {
            auto& trip = getTrip(feed, tripId);

            // each trip contains a range of stopevents :
            for (auto const& stoptime : trip.getStopTimes()) {
                int departureTime = stoptime.getDepartureTime().seconds();
                int arrivalTime = stoptime.getArrivalTime().seconds();
                stopEvents.emplace_back(arrivalTime, departureTime);
            }

            size_t nbStopEventsInThisTrip = trip.getStopTimes().size();
            nbStopEventsInThisRoute += nbStopEventsInThisTrip;
        }

        firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;
        currentRouteFirstStopEvent += nbStopEventsInThisRoute;
    }

    // From now on :
    //      routeRank = number of routes
    //      currentRouteFirstStopEvent = number of stopevents in all the routes
    // Setting past-the-end stopEvents :
    firstStopEventOfRoute[routeRank] = currentRouteFirstStopEvent;

    return {stopEvents, firstStopEventOfRoute};
}

static pair<vector<RAPTOR::RouteSegment>, vector<size_t>> convert_routeSegmentsRelated(
    vector<RAPTOR::Route> const& routeData,
    unordered_map<string, size_t> const& stopidToRank,
    unordered_map<my::preprocess::RouteLabel, size_t> const& routeToRank) {
    // building an intermediate structure that associates a stopRank to all its routes
    // for a given stop, this structures stores some pairs {route + stop index in this route}

    auto nb_stops = stopidToRank.size();
    vector<vector<pair<my::preprocess::RouteLabel, int>>> routesUsingAStop(nb_stops);

    for (auto& route : routeData) {
        my::preprocess::RouteLabel routeLabel = route.name;
        vector<string> stopsOfThisRoute = routeLabel.toStopIds();

        for (size_t stopIndex = 0; stopIndex < stopsOfThisRoute.size(); ++stopIndex) {
            string const& stopid = stopsOfThisRoute[stopIndex];
            size_t stopRank = stopidToRank.at(stopid);

            vector<pair<my::preprocess::RouteLabel, int>>& routesUsingThisStop = routesUsingAStop[stopRank];
            routesUsingThisStop.emplace_back(routeLabel, stopIndex);
        }
    }

    // From now on, for each stop, we know the routes that uses it

    // TODO = check that each route only appears once for a given stop

    vector<size_t> firstRouteSegmentOfStop(stopidToRank.size() + 1);
    vector<RAPTOR::RouteSegment> routeSegments;

    size_t currentStopFirstRouteSegment = 0;

    for (size_t stopRank = 0; stopRank < routesUsingAStop.size(); ++stopRank) {
        auto& routesUsingThisStop = routesUsingAStop[stopRank];

        for (auto & [ routeId, stopIndexInThisRoute ] : routesUsingThisStop) {
            auto routeRank = static_cast<u_int32_t>(routeToRank.at(routeId));
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


my::preprocess::UltraGtfsData::UltraGtfsData(string const& gtfsFolder) {
    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfsFolder);

    GtfsParsedData gtfs{feed};

    // use GTFS parsed data to build ULTRA data :
    routeData = build_routeData(gtfs.rankedRoutes);
    stopData = build_stopData(gtfs.rankedStops);
    tie(stopIds, firstStopIdOfRoute) = build_stopIdsRelated(routeData, gtfs.stopidToRank);
    tie(stopEvents, firstStopEventOfRoute) = build_stopEventsRelated(routeData, gtfs.routeToTrips, feed);
    tie(routeSegments, firstRouteSegmentOfStop) =
        convert_routeSegmentsRelated(routeData, gtfs.stopidToRank, gtfs.routeToRank);

    // STUB : according to some comments in ULTRARAPTOR.h, buffer times have to be implicit :
    implicitDepartureBufferTimes = true;
    implicitArrivalBufferTimes = true;
}


void my::preprocess::UltraGtfsData::dump(string const& filename) const {
    IO::serialize(filename, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
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

    IO::deserialize(tmpfile.file, freshFirstRouteSegmentOfStop, freshFirstStopIdOfRoute, freshFirstStopEventOfRoute, freshRouteSegments, freshStopIds, freshStopEvents, freshStopData, freshRouteData, freshImplicitDepartureBufferTimes, freshImplicitArrivalBufferTimes);

    bool areFirstRouteSegmentOfStopEqual = firstRouteSegmentOfStop == freshFirstRouteSegmentOfStop;
    bool areFirstStopIdOfRouteEqual = firstStopIdOfRoute == freshFirstStopIdOfRoute;
    bool areFirstStopEventOfRouteEqual = firstStopEventOfRoute == freshFirstStopEventOfRoute;
    bool areRouteSegmentsEqual = equal(
        routeSegments.begin(),
        routeSegments.end(),
        freshRouteSegments.begin(),
        [](auto const& x, auto const& y) { return x.routeId == y.routeId && x.stopIndex == y.stopIndex; }
    );
    bool areStopIdsEqual = stopIds == freshStopIds;
    bool areStopEventsEqual = equal(
        stopEvents.begin(),
        stopEvents.end(),
        freshStopEvents.begin(),
        [](auto const& x, auto const& y) { return x.arrivalTime == y.arrivalTime && x.departureTime == y.departureTime; }
    );
    bool areStopDataEqual = equal(
        stopData.begin(),
        stopData.end(),
        freshStopData.begin(),
        [](auto const& x, auto const& y) { return x.name == y.name && x.coordinates == y.coordinates && x.minTransferTime == y.minTransferTime; }
    );
    bool areRouteDataEqual = equal(
        routeData.begin(),
        routeData.end(),
        freshRouteData.begin(),
        [](auto const& x, auto const& y) { return x.name == y.name && x.type == y.type; }
    );
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


    return (
        areFirstRouteSegmentOfStopEqual &&
        areFirstStopIdOfRouteEqual &&
        areFirstStopEventOfRouteEqual &&
        areRouteSegmentsEqual &&
        areStopIdsEqual &&
        areStopEventsEqual &&
        areStopDataEqual &&
        areRouteDataEqual &&
        areImplicitDepartureBufferTimesEqual &&
        areImplicitArrivalBufferTimesEqual
    );
}

void UltraGtfsData::serialize(const string& fileName) const {
    IO::serialize(fileName, firstRouteSegmentOfStop, firstStopIdOfRoute, firstStopEventOfRoute, routeSegments, stopIds, stopEvents, stopData, routeData, implicitDepartureBufferTimes, implicitArrivalBufferTimes);
}

}
