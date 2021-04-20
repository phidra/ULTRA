#pragma once

#include <istream>
#include "MutualizedPreprocess/polygon.h"

namespace my::preprocess {

static const std::string NO_POLYGON = "NONE";

BgPolygon get_polygon(std::string polygonfile_path);

}
