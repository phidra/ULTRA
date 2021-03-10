#pragma once

#include <vector>

#include "../../DataStructures/RAPTOR/Entities/Stop.h"
#include "../../DataStructures/RAPTOR/Entities/Route.h"
#include "../../DataStructures/RAPTOR/Entities/StopEvent.h"
#include "../../DataStructures/RAPTOR/Entities/RouteSegment.h"
#include "../../Helpers/Types.h"

namespace my {

//From a given GTFS feed, builds the RAPTOR binary expected by ULTRA :
struct UltraGtfsData {
    UltraGtfsData(std::string const& gtfsFolder);
    void dump(std::string const& filename) const;
    bool checkSerializationIdempotence() const;

    std::vector<size_t> firstRouteSegmentOfStop;
    std::vector<size_t> firstStopIdOfRoute;
    std::vector<size_t> firstStopEventOfRoute;
    std::vector<RAPTOR::RouteSegment> routeSegments;
    std::vector<::StopId> stopIds;
    std::vector<RAPTOR::StopEvent> stopEvents;
    std::vector<RAPTOR::Stop> stopData;
    std::vector<RAPTOR::Route> routeData;
    bool implicitDepartureBufferTimes;
    bool implicitArrivalBufferTimes;

    // mirroring RAPTOR::Data serialization :
    void serialize(const std::string& fileName) const;
};

}  // namespace my
