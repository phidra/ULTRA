#pragma once

#include <vector>
#include <istream>

#include "../Common/types.h"

namespace my {

std::vector<Stop> parse_stopfile(const char* stopfile, std::istream& stopfile_stream);

}
