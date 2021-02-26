#pragma once

#include <vector>

#include "../DataStructures/Graph/Graph.h"
#include "Common/graphtypes.h"

namespace my {

TransferGraph buildTransferGraph(
    std::vector<my::Edge> const& edgesWithStops,
    std::vector<my::StopWithClosestNode> const& stopsWithClosestNode);

}
