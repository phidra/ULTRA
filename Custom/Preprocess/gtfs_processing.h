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
// NOTE : to enforce different name than those of ULTRA, we use xxxID instead of xxxId
//  - StopId -> this is an ULTRA structure
//  - StopID -> this is a prepared GTFS structure (built by this code)

// this is the "scientific" route used in ULTRA code :
using RouteLabel = std::string;
using TripLabel = std::string;
using StopID = std::string;

std::vector<StopID> routeToStops(RouteLabel const& route);
std::map<RouteLabel, std::set<TripLabel>> partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<RouteLabel>, std::unordered_map<RouteLabel, size_t>> rankRoutes(std::map<RouteLabel, std::set<TripLabel>> const& routeToTrips);
std::pair<std::vector<StopID>, std::unordered_map<StopID, size_t>> rankStops(std::map<RouteLabel, std::set<TripLabel>> const& routeToTrips);

bool checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed, std::map<RouteLabel, std::set<TripLabel>> const& partition);

}
