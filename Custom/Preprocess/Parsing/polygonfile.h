#pragma once

#include <istream>
#include "../../Common/polygon.h"

static const std::string NO_POLYGON = "NONE";

my::BgPolygon get_polygon(std::string polygonfile_path);
