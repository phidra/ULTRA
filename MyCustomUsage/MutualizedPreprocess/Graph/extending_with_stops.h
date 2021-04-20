#pragma once

#include <vector>

#include "MutualizedPreprocess/types.h"
#include "MutualizedPreprocess/graphtypes.h"

namespace my::preprocess {

std::pair<std::vector<Edge>, std::vector<StopWithClosestNode>> extend_graph(std::vector<Stop> const& stops,
                                                                           std::vector<Edge> const& edgesOSM,
                                                                           float walkspeed_km_per_h);

}
