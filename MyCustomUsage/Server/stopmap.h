#pragma once

#include <unordered_map>
#include <string>

namespace myserver {

struct Stop {
    inline Stop(std::string id_, std::string name_, double lon_, double lat_)
        : id{id_}, name{name_}, lon(lon_), lat(lat_) {}
    std::string id;
    std::string name;
    double lon;
    double lat;
};

using StopMap = std::unordered_map<std::string, Stop>;

inline std::string stopid_to_stopname(std::string const& stop_id, StopMap const& stops, std::string fallback) {
    try {
        Stop const& stop = stops.at(stop_id);
        return stop.name;
    } catch (std::out_of_range&) {
        return fallback;
    }
}

}  // namespace myserver
