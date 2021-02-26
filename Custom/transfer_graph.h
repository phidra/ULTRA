#pragma once

#include <vector>
#include <filesystem>

#include "../DataStructures/Graph/Graph.h"
#include "Common/graphtypes.h"

namespace my {

TransferGraph buildTransferGraph(
    std::vector<my::Edge> const& edgesWithStops,
    std::vector<my::StopWithClosestNode> const& stopsWithClosestNode);

struct UltraTransferData {

    UltraTransferData(std::filesystem::path osmFile, std::filesystem::path polygonFile, std::filesystem::path gtfsStopfile);
    TransferGraph transferGraph;
};

}
