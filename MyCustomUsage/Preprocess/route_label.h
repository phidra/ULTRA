#pragma once

#include <vector>
#include <functional>

#include "ad/cppgtfs/Parser.h"

namespace my::preprocess {

// NOTE : to enforce different name than those of ULTRA, we use xxxLabel instead of xxxId
//  - ::StopId -> this is an ULTRA structure
//  - StopLabel -> this is a my::preprocess GTFS structure (defined by this code)
using TripLabel = std::string;
using StopLabel = std::string;
struct RouteLabel {
    RouteLabel() = default;
    RouteLabel(std::string const& label_) : label{label_} {}
    static RouteLabel fromTrip(ad::cppgtfs::gtfs::Trip const& trip);
    std::vector<StopLabel> toStops() const;
    operator std::string() const { return label; }
    bool operator<(std::string const& other) const { return label < other; }
    bool operator==(std::string const& other) const { return label == other; }
    std::string label;
};

}  // namespace my::preprocess


namespace std
{

template <> struct hash<my::preprocess::RouteLabel>
{
    size_t operator() (my::preprocess::RouteLabel const& x) const { return hash<string>()(x.label); }
};

}  // namespace std
