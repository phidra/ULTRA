#pragma once

#include <sstream>

namespace myserver {

struct InconsistentLegDurations : public std::exception {
    InconsistentLegDurations(int start_time_,
                             int departure_time_,
                             int arrival_time_,
                             int full_duration_,
                             int waiting_duration_,
                             int traveling_duration_)
        : start_time{start_time_},
          departure_time{departure_time_},
          arrival_time{arrival_time_},
          full_duration{full_duration_},
          waiting_duration{waiting_duration_},
          traveling_duration{traveling_duration_} {
        std::ostringstream oss;
        oss << "Inconsistent leg durations/times : [";
        oss << "start_time=" << start_time << "|";
        oss << "departure_time=" << departure_time << "|";
        oss << "arrival_time=" << arrival_time << "|";
        oss << "full_duration=" << full_duration << "|";
        oss << "waiting_duration=" << waiting_duration << "|";
        oss << "traveling_duration=" << traveling_duration;
        oss << "]";
        error_msg = oss.str();
    }

    inline const char* what() const throw() { return error_msg.c_str(); }

    int start_time;
    int departure_time;
    int arrival_time;
    int full_duration;
    int waiting_duration;
    int traveling_duration;
    std::string error_msg;
};

}
