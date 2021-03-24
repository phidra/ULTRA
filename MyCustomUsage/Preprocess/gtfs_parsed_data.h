#pragma once

#include <vector>
#include <functional>

#include "ad/cppgtfs/Parser.h"

#include "route_label.h"

// From a given GTFS feed, GtfsParsedData is an abstraction of the GTFS data, suitable for ULTRA :
//  - trips are partitionned into "scientific" routes (see details below)
//  - routes and stops are ranked (to be able to store them in a vector)
// NOTE : this implementation is tightly coupled to the library used to parse GTFS : cppgtfs


// WARNING : there are two mismatching definitions of the word "route" :
//  - what scientific papers calls "route" is a particular set of stops
//    in particular, if two trips travel between exactly the same stops, they belong to the same route.
//  - what GTFS standard (and thus cppgtfs) calls "route" is just a given structure associated to a trip
//    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
//    even if they don't use exactly the same set of stops
//
// BEWARE : the "routes" returned by cppgtfs are not the scientific ones !
// In general, in ULTRA code (and in code building ULTRA data), the "routes" are the scientific ones.
// Thus, one of the purpose of GtfsParsedData is to build "scientific" routes from GTFS feed.


namespace my::preprocess {

// amongst the trips of a given route, we want to order trips by their departure time
// thus, we use a std::pair to achieve that, as they will compare using the left element of the pair :
using TripDepartureTime = int;  // departure time is represented in number of seconds
using OrderableTripLabel = std::pair<TripDepartureTime, TripLabel>;


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
