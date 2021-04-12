#pragma once

#include <vector>
#include <functional>

#include "ad/cppgtfs/Parser.h"

// this module defines the structures used to store GTFS data after parsing.

namespace my::preprocess {

// amongst the trips of a given route, we want to order trips by their departure time
// to achieve that, we use a std::pair, as the pair will compare using its left element :
using TripEventTime = int;  // departure/arrival times are represented in number of seconds
using OrderableTripId = std::pair<TripEventTime, std::string>;

// A RouteLabel is a wrapper around a string that stores the concatenation of the route's stop ids
struct RouteLabel {
    RouteLabel() = default;
    RouteLabel(std::string const& label_) : label{label_} {}
    std::vector<std::string> toStopIds() const;
    operator std::string() const { return label; }
    bool operator<(std::string const& other) const { return label < other; }
    bool operator==(std::string const& other) const { return label == other; }
    std::string label;
};


// A ParsedRoute stores the trips (and their events) of a route
struct ParsedRoute {
    using StopEvent = std::pair<int, int>;  // arrival, departure
    std::map<OrderableTripId, std::vector<StopEvent>> trips;  // the map ensures trips are ordered

    void addTrip(OrderableTripId const& tripId, ad::cppgtfs::gtfs::Trip const& trip);
};


// A ParsedStop stores what is necessary to ultra : name and coordinates :
struct ParsedStop {
    std::string id;
    std::string name;
    double latitude;
    double longitude;

    ParsedStop(std::string const& id_, std::string const & name_, double latitude_, double longitude_);
    std::string as_string() const;
};

}  // namespace my::preprocess


namespace std
{

template <> struct hash<my::preprocess::RouteLabel>
{
    size_t operator() (my::preprocess::RouteLabel const& x) const { return hash<string>()(x.label); }
};

}  // namespace std