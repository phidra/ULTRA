#pragma once

#include <string>
#include "exceptions.h"

struct Leg {
    Leg(bool is_walk_,
        std::string departure_id_,
        std::string arrival_id_,
        int start_time_,
        int departure_time_,
        int arrival_time_,
        std::vector<std::string> stops_)
        : is_walk{is_walk_},
          departure_id{departure_id_},
          arrival_id{arrival_id_},
          start_time{start_time_},
          departure_time{departure_time_},
          arrival_time{arrival_time_},
          stops{stops_} {}

    bool is_walk;
    std::string departure_id;

    // leg has several times / durations, because it may include some waiting before the traveling :
    int start_time;      // leg's start_time is either the full journey's departure_time, or the arrival_time of the
                         // previous leg
    int departure_time;  // time at which we really begin to travel (= start_time + waiting_duration)
    // for walk, waiting_duration=0, and thus full_duration == traveling_duration
    // for PT, full_duration = waiting_duration + traveling_duration
    std::string arrival_id;
    int arrival_time;
    std::vector<std::string> stops;

    inline int get_full_duration() const { return arrival_time - start_time; }
    inline int get_waiting_duration() const { return departure_time - start_time; }
    inline int get_traveling_duration() const { return arrival_time - departure_time; }

    inline void sanity_check() {
        bool expected = (get_full_duration() == get_waiting_duration() + get_traveling_duration() && start_time >= 0 &&
                         departure_time >= 0 && arrival_time >= 0 && get_full_duration() >= 0 &&
                         get_waiting_duration() >= 0 && get_traveling_duration() >= 0);
        if (!expected) {
            throw InconsistentLegDurations(start_time, departure_time, arrival_time, get_full_duration(),
                                           get_waiting_duration(), get_traveling_duration());
        }
    }

    inline std::string as_string() const {
        std::ostringstream oss;
        oss << "[" << (is_walk ? "walk" : " tc ") << "]";
        oss << "  FROM=" << departure_id << "  ->  TO=" << arrival_id;
        oss << "  (start at " << start_time << ", begins travel at " << departure_time << "  ....  arrival at "
            << arrival_time << ")";
        return oss.str();
    }
};
