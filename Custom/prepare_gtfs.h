#pragma once

#include <string>
#include <vector>
#include <map>

#include "ad/cppgtfs/Parser.h"

namespace my {

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
// NOTE : to enforce different name than those of ULTRA, we use xxxID instead of xxxId
//  - StopId -> this is an ULTRA structure
//  - StopID -> this is a prepared GTFS structure (built by this code)

// this is the "scientific" route used in ULTRA code :
using RouteID = std::string;
// this is the id of a "route" in the GTFS data (barely used in ULTRA code) :
using GtfsRouteID = std::string;

using TripID = std::string;
using RouteId = std::string;

using StopID = std::string;

std::vector<StopID> routeToStops(RouteID const& route);
std::map<RouteID, std::set<TripID>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<RouteID>, std::unordered_map<RouteID, size_t>> rankRoutes(std::map<RouteID, std::set<TripID>> const& routeToTrips);
std::pair<std::vector<StopID>, std::unordered_map<StopID, size_t>> rankStops(std::map<RouteID, std::set<TripID>> const& routeToTrips);

}
