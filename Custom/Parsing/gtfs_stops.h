#pragma once

#include <vector>
#include <istream>

#include "../Common/types.h"

namespace my {

std::vector<Stop> parse_gtfs_stops(const char* gtfs_stopfile, std::istream& stopfile_stream);

}
