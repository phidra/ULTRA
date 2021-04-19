#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <tuple>
#include <cassert>

namespace my {

inline std::tuple<int, int, int> dispatch(int duration) {
    int nb_hours = duration / 3600;
    int duration_without_hours = duration - (nb_hours*3600);
    int nb_minutes = duration_without_hours / 60;
    int duration_without_hours_or_minutes = duration_without_hours - (nb_minutes*60);
    int nb_seconds = duration_without_hours_or_minutes;
    assert(nb_seconds >= 0 && nb_seconds < 60);
    assert(nb_minutes >= 0 && nb_minutes < 60);
    assert(nb_hours >= 0);
    return std::make_tuple(nb_hours, nb_minutes, nb_seconds);
}

inline std::string format_time(int duration) {
    // INPUT  = 49272
    // OUTPUT = "13:41:12"

    int nb_hours, nb_minutes, nb_seconds;
    std::tie(nb_hours, nb_minutes, nb_seconds) = dispatch(duration);

    std::ostringstream oss;
    oss << std::setfill('0');
    oss << std::setw(2) << nb_hours << ":";
    oss << std::setw(2) << nb_minutes << ":";
    oss << std::setw(2) << nb_seconds;
    return oss.str();
}

inline std::string format_duration(int duration) {
    // INPUT  = 49272
    // OUTPUT = "13h41m12s"
    // INPUT  = 2472
    // OUTPUT = "41m12s"

    int nb_hours, nb_minutes, nb_seconds;
    std::tie(nb_hours, nb_minutes, nb_seconds) = dispatch(duration);

    std::ostringstream oss;
    oss << std::setfill('0');

    // the last time to display only without hours is 59m59s = 3599
    if (duration <= 3599) {
        oss << std::setw(2) << nb_minutes << "m";
        oss << std::setw(2) << nb_seconds << "s";
    }
    else {
        oss << nb_hours << "h";
        oss << std::setw(2) << nb_minutes << "m";
        oss << std::setw(2) << nb_seconds << "s";
    }

    return oss.str();
}

}
