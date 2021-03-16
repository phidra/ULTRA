#pragma once

#include <string>
#include <vector>
#include <map>

#include "ad/cppgtfs/Parser.h"

// this code process GTFS feed to ease conversion to ULTRA binary format

namespace my::preprocess {

// About the word "route" :
//  - what science papers calls "route" is a particular group of stops
//    in particular, if two trips travel between the same stops, they have the same route.
//  - what GTFS standard calls "route" is just a given structure associated to a trip
//    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
//    even if they don't use exactly the same set of stops
//
// In ULTRA code, the only meaning of the term "Route" is the scientific one.
// If neeeded, the "route" used in GTFS standard is called GtfsRoute.
//
// NOTE : to enforce different name than those of ULTRA, we use xxxLabel instead of xxxId
//  - ::StopId -> this is an ULTRA structure
//  - StopLabel -> this is a my::preprocess GTFS structure (built by this code)

// this is the "scientific" route used in ULTRA code :
using RouteLabel = std::string;
using TripLabel = std::string;
using StopLabel = std::string;

// amongst the trips of a given route, we want to order trips by their departure time
// as std::pair is naturally ordered (starting with left element of the pair), we use it to order trips :
using TripDepartureTime = int;  // departure time is represented in number of seconds
using OrderedTripLabel = std::pair<TripDepartureTime, TripLabel>;

std::vector<StopLabel> routeToStops(RouteLabel const& route);
std::map<RouteLabel, std::set<OrderedTripLabel>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<RouteLabel>, std::unordered_map<RouteLabel, size_t>> rankRoutes(std::map<RouteLabel, std::set<OrderedTripLabel>> const& routeToTrips);
std::pair<std::vector<StopLabel>, std::unordered_map<StopLabel, size_t>> rankStops(std::map<RouteLabel, std::set<OrderedTripLabel>> const& routeToTrips);

bool checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, std::map<RouteLabel, std::set<OrderedTripLabel>> const& partition);

}
