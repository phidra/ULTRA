#pragma once

#include <unordered_set>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../DataStructures/RAPTOR/Entities/RouteSegment.h"

#include "ad/cppgtfs/Parser.h"

#include "prepare_gtfs.h"

//From a given GTFS feed, this code helps to build the RAPTOR binary expected by ULTRA.

namespace my {

std::vector<RAPTOR::Route> build_routeData(std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips);
std::vector<RAPTOR::Stop> build_stopData(std::vector<my::StopID> const& rankedStops, ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<StopId>, std::vector<size_t>> build_stopIdsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stopToRank);
std::pair<std::vector<RAPTOR::StopEvent>, std::vector<size_t>> build_stopEventsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips,
    ad::cppgtfs::gtfs::Feed const& feed);
std::pair<std::vector<RAPTOR::RouteSegment>, std::vector<size_t>> convert_routeSegmentsRelated(
    std::vector<RAPTOR::Route> const& routeData,
    std::unordered_map<my::StopID, size_t> const& stopToRank,
    std::unordered_map<my::RouteID, size_t> const& routeToRank,
    std::map<my::RouteID, std::set<my::TripID>> const& routeToTrips);

void convert_gtfs_to_ultra_binary(ad::cppgtfs::gtfs::Feed const& feed, std::string const& outputFileName, bool checkIdempotence);

}  // namespace my
