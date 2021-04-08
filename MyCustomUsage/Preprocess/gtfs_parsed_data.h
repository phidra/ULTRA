#pragma once

#include <vector>
#include <functional>

#include "ad/cppgtfs/Parser.h"

#include "route_label.h"

// From a given GTFS feed, GtfsParsedData is an abstraction of the GTFS data, suitable for ULTRA :
//  - only the stops that appear in at least one trip are kept (unused stops are ignored)
//  - trips are partitionned into "scientific" routes (about routes, see details below)
//  - routes and stops are ranked (about ranks, see details below)
//  - a route (or a stop) can be identified with either its "ID" (RouteLabel/StopId) or its rank
//  - the conversion between ID<->rank is done with the conversion structures

// NOTE : this implementation is tightly coupled to the library used to parse GTFS : cppgtfs

// WARNING : there are two mismatching definitions of the word "route" :
//  - what scientific papers calls "route" is a particular set of stops
//    in particular, if two trips travel between exactly the same stops, they belong to the same route.
//  - what GTFS standard (and thus cppgtfs) calls "route" is just a given structure associated to a trip
//    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
//    even if they don't use exactly the same set of stops
//
// BEWARE : the "routes" returned by cppgtfs are not the scientific ones, and are not further used !
// In general, in ULTRA code (and in code building ULTRA data), the "routes" are the scientific ones.
// Thus, one of the purpose of GtfsParsedData is to build "scientific" routes from GTFS feed.


namespace my::preprocess {

// amongst the trips of a given route, we want to order trips by their departure time
// thus, we use a std::pair to achieve that, as they will compare using the left element of the pair :
using TripDepartureTime = int;  // departure time is represented in number of seconds
using OrderableTripLabel = std::pair<TripDepartureTime, TripLabel>;


struct ParsedStop {
    std::string id;
    std::string name;
    double latitude;
    double longitude;

    ParsedStop(std::string const& id_, std::string const & name_, double latitude_, double longitude_) :
        id{id_},
        name{name_},
        latitude{latitude_},
        longitude{longitude_}
    {}

    std::string as_string() const {
        std::ostringstream oss;
        oss << "ParsedStop{" << id << ", " << name << ", " << latitude << ", " << longitude << "}";
        return oss.str();
    };
};


struct GtfsParsedData {
    GtfsParsedData(ad::cppgtfs::gtfs::Feed const&);

    std::map<RouteLabel, std::set<OrderableTripLabel>> routeToTrips;


    // stops and routes are ranked
    // each stop/route has an arbitrary rank from 0 to N-1 (where N is the number of stops/routes)
    // this rank will be used to store the stops/routes in a vector
    // the following conversion structures allow to convert between rank and label :

    //   - rankedRoutes associates a rank to a route
    //   - routeToRank allows to retrieve the rank of a given route
    std::vector<RouteLabel> rankedRoutes;
    std::unordered_map<RouteLabel, size_t> routeToRank;


    //   - rankedStops associates a rank to a stop
    //   - stopidToRank allows to retrieve the rank of a given stop
    std::vector<ParsedStop> rankedStops;
    std::unordered_map<std::string, size_t> stopidToRank;


};

}
