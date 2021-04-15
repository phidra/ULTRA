#pragma once

#include <vector>

#include "Common/types.h"
#include "Common/graphtypes.h"

namespace my::preprocess {

std::pair<std::vector<Edge>, std::vector<StopWithClosestNode>> extend_graph(std::vector<Stop> const& stops,
                                                                           std::vector<Edge> const& edgesOSM,
                                                                           float walkspeed_km_per_h);

}
