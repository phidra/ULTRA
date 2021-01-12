#include <iostream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Data.h"

inline void usage() noexcept {
    std::cout << "Usage: binary-analyzer <RAPTOR binary>" << std::endl;
    exit(0);
}

void dump_geojson_stops(std::ostream& out, RAPTOR::Data& data) {
    /* for (auto const& stop: data.stopsOfRoute(RouteId(42))) { */
    /* } */
    /* inline SubRange<std::vector<StopId>> stopsOfRoute(const RouteId route) const noexcept { */
    /*     AssertMsg(isRoute(route), "The id " << route << " does not represent a route!"); */
    /*     return SubRange<std::vector<StopId>>(stopIds, firstStopIdOfRoute, route); */

    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    for (auto const& stop : data.stopsOfRoute(RouteId(42))) {
        /* // coordinates : */
        /* rapidjson::Value coordinates(rapidjson::kArrayType); */
        /* coordinates.PushBack(rapidjson::Value().SetDouble(stop.lon), a); */
        /* coordinates.PushBack(rapidjson::Value().SetDouble(stop.lat), a); */

        /* // geometry : */
        /* rapidjson::Value geometry(rapidjson::kObjectType); */
        /* geometry.AddMember("coordinates", coordinates, a); */
        /* geometry.AddMember("type", "Point", a); */

        /* // properties : */
        /* rapidjson::Value properties(rapidjson::kObjectType); */
        /* properties.AddMember("stop_id", rapidjson::Value().SetString(stop.id.c_str(), a), a); */
        /* properties.AddMember("stop_name", rapidjson::Value().SetString(stop.name.c_str(), a), a); */
        /* properties.AddMember("closest_node_id", rapidjson::Value().SetString(stop.closest_node_id.c_str(), a), a); */
        /* properties.AddMember("closest_node_url", rapidjson::Value().SetString(stop.closest_node_url.c_str(), a), a);
         */

        /* // feature : */
        /* rapidjson::Value feature(rapidjson::kObjectType); */
        /* feature.AddMember("type", "Feature", a); */
        /* feature.AddMember("geometry", geometry, a); */
        /* feature.AddMember("properties", properties, a); */
        /* features.PushBack(feature, a); */
    }

    doc.AddMember("features", features, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();
    const std::string raptorFile = argv[1];
    std::cout << "Analyzing : " << raptorFile << std::endl;
    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();

    std::cout << "Tailles : " << std::endl;
    std::cout << "stopData                     = " << data.stopData.size() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "firstRouteSegmentOfStop      = " << data.firstRouteSegmentOfStop.size() << std::endl;
    std::cout << "routeSegments                = " << data.routeSegments.size() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "routeData                    = " << data.routeData.size() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "firstStopIdOfRoute           = " << data.firstStopIdOfRoute.size() << std::endl;
    std::cout << "stopIds                      = " << data.stopIds.size() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "firstStopEventOfRoute        = " << data.firstStopEventOfRoute.size() << std::endl;
    std::cout << "stopEvents                   = " << data.stopEvents.size() << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "implicitDepartureBufferTimes = " << std::boolalpha << data.implicitDepartureBufferTimes << std::endl;
    std::cout << "implicitArrivalBufferTimes   = " << std::boolalpha << data.implicitArrivalBufferTimes << std::endl;
    std::cout << std::endl;

    // investigation sur une route quelconque (e.g. la 42) :
    auto route42 = data.routeData[42];
    std::cout << "La route n°42 est : " << route42;
    std::cout << "Elle contient #stops = " << data.numberOfStopsInRoute(RouteId(42)) << std::endl;
    std::cout << "Elle contient #trips = " << data.numberOfTripsInRoute(RouteId(42)) << std::endl;
    std::cout << "Elle contient #stopEvents = " << data.numberOfStopEventsInRoute(RouteId(42)) << std::endl;
    int counter = 0;
    for (auto const& se : data.stopEventsOfRoute(RouteId(42))) {
        std::cout << "\t[" << counter++ << "] " << se.departureTime << std::endl;
    }

    // Dumping d'un geojson d'une route donnée :
    //  - liste de stops
    dump_geojson_stops(std::cout, data);
    return 0;
}
