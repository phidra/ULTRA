#pragma once

#include <vector>
#include <filesystem>

#include "DataStructures/Graph/Graph.h"
#include "DataStructures/RAPTOR/Entities/Stop.h"
#include "MutualizedPreprocess/Graph/walking_graph.h"

namespace my::preprocess {

struct UltraTransferData {
    UltraTransferData(WalkingGraph&&);
    static bool areApproxEqual(TransferGraph const& left, TransferGraph const& right);
    bool checkSerializationIdempotence() const;

    WalkingGraph walkingGraph;
    TransferGraph transferGraphUltra;  // this is from ULTRA code (unfortunately, in the global namespace)
};

}  // namespace my::preprocess
