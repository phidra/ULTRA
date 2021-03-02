#pragma once

#include <vector>
#include <istream>

#include "../Common/types.h"

namespace my {

struct BadStopFile : public std::exception {
    std::string msg;
    BadStopFile(std::string const& stopfile) : msg{std::string("ERROR: unable to read stopfile : '") + stopfile} {}
    inline const char* what() const throw() { return msg.c_str(); }
};

std::vector<Stop> parse_gtfs_stops(const char* gtfs_stopfile, std::istream& stopfile_stream);

}
