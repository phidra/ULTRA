#pragma once

#include <string>
#include <vector>
#include <map>

#include "ad/cppgtfs/Parser.h"

// this code processes a GTFS feed to ease the building of an ULTRA binary format

namespace my::preprocess {

// NOTE : to enforce different name than those of ULTRA, we use xxxLabel instead of xxxId
//  - ::StopId -> this is an ULTRA structure
//  - StopLabel -> this is a my::preprocess GTFS structure (defined by this code)
using RouteLabel = std::string;
using TripLabel = std::string;
using StopLabel = std::string;

// amongst the trips of a given route, we want to order trips by their departure time
// thus, we use a std::pair to achieve that, as they will compare using the left element of the pair :
using TripDepartureTime = int;  // departure time is represented in number of seconds
using OrderableTripLabel = std::pair<TripDepartureTime, TripLabel>;

std::map<RouteLabel, std::set<OrderableTripLabel>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<RouteLabel>, std::unordered_map<RouteLabel, size_t>> rankRoutes(std::map<RouteLabel, std::set<OrderableTripLabel>> const& routeToTrips);
std::pair<std::vector<StopLabel>, std::unordered_map<StopLabel, size_t>> rankStops(std::map<RouteLabel, std::set<OrderableTripLabel>> const& routeToTrips);
std::vector<StopLabel> routeToStops(RouteLabel const& route);

bool checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, std::map<RouteLabel, std::set<OrderableTripLabel>> const& partition);

}
