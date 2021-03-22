#pragma once

#include <vector>
#include <functional>

#include "ad/cppgtfs/Parser.h"

#include "route_label.h"


namespace my::preprocess {

// amongst the trips of a given route, we want to order trips by their departure time
// thus, we use a std::pair to achieve that, as they will compare using the left element of the pair :
using TripDepartureTime = int;  // departure time is represented in number of seconds
using OrderableTripLabel = std::pair<TripDepartureTime, TripLabel>;


// From a given GTFS feed, prepare the GTFS data :
// TODO : describe
// This implementation is tightly coupled to the library used to parse GTFS : cppgtfs

struct GtfsParsedData {
    GtfsParsedData(ad::cppgtfs::gtfs::Feed const&);

    std::map<RouteLabel, std::set<OrderableTripLabel>> routeToTrips;

    //   - rankedRoutes associates a rank to a route
    //   - routeToRank allows to retrieve the rank of a given route
    std::vector<RouteLabel> rankedRoutes;
    std::unordered_map<RouteLabel, size_t> routeToRank;


    //   - rankedStops associates a rank to a stop
    //   - stopToRank allows to retrieve the rank of a given stop
    std::vector<StopLabel> rankedStops;
    std::unordered_map<StopLabel, size_t> stopToRank;

};

}
