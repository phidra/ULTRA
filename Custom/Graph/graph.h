#pragma once

#include <vector>

#include "../Common/graphtypes.h"
#include "../Common/polygon.h"

namespace my {

std::vector<Edge> osm_to_graph(std::string osmfile, my::BgPolygon polygon, float walkspeed_km_per_h);

}
