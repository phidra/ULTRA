#include <iostream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include "../Algorithms/RAPTOR/ULTRARAPTOR.h"

#include "../DataStructures/RAPTOR/Data.h"

inline void usage() noexcept {
    std::cout << "Usage: binary-analyzer <RAPTOR binary>" << std::endl;
    exit(0);
}

using ShortcutRAPTOR = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

rapidjson::Value stop_to_coordinates(RAPTOR::Stop const& stop, rapidjson::Document::AllocatorType& a) {
    rapidjson::Value json_location(rapidjson::kArrayType);
    json_location.PushBack(rapidjson::Value(stop.coordinates.longitude), a);
    json_location.PushBack(rapidjson::Value(stop.coordinates.latitude), a);
    return json_location;
}

void dump_journey(std::ostream& out, RAPTOR::Data& data, std::vector<StopId> path) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    // coordinates :
    rapidjson::Value coordinates(rapidjson::kArrayType);

    for (auto const& stop_id : path) {
        auto const& stop = data.stopData[stop_id];
        auto stop_coords = stop_to_coordinates(stop, a);
        coordinates.PushBack(stop_coords, a);
    }

    // geometry :
    rapidjson::Value geometry(rapidjson::kObjectType);
    geometry.AddMember("coordinates", coordinates, a);
    geometry.AddMember("type", "LineString", a);

    // properties :
    rapidjson::Value properties(rapidjson::kObjectType);
    /* properties.AddMember("stop_id", rapidjson::Value(stop_id), a); */
    /* properties.AddMember("stop_name", rapidjson::Value().SetString(stop.name.c_str(), a), a); */

    // feature :
    rapidjson::Value feature(rapidjson::kObjectType);
    feature.AddMember("type", "Feature", a);
    feature.AddMember("geometry", geometry, a);
    feature.AddMember("properties", properties, a);
    features.PushBack(feature, a);

    doc.AddMember("features", features, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);
}

void dump_route_stops(std::ostream& out, RAPTOR::Data& data, RouteId route) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    size_t stop_counter = 0;
    for (auto const& stop_index : data.stopsOfRoute(route)) {
        auto const& stop = data.stopData[stop_index];

        // coordinates :
        rapidjson::Value coordinates(rapidjson::kArrayType);
        coordinates.PushBack(rapidjson::Value().SetDouble(stop.coordinates.longitude), a);
        coordinates.PushBack(rapidjson::Value().SetDouble(stop.coordinates.latitude), a);

        // geometry :
        rapidjson::Value geometry(rapidjson::kObjectType);
        geometry.AddMember("coordinates", coordinates, a);
        geometry.AddMember("type", "Point", a);

        // properties :
        rapidjson::Value properties(rapidjson::kObjectType);
        properties.AddMember("stop_index", rapidjson::Value(stop_index), a);
        properties.AddMember("stop_counter", rapidjson::Value(stop_counter++), a);
        properties.AddMember("stop_name", rapidjson::Value().SetString(stop.name.c_str(), a), a);

        // feature :
        rapidjson::Value feature(rapidjson::kObjectType);
        feature.AddMember("type", "Feature", a);
        feature.AddMember("geometry", geometry, a);
        feature.AddMember("properties", properties, a);
        features.PushBack(feature, a);
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
    /* for (auto const& se : data.stopEventsOfRoute(RouteId(42))) { */
    /*     std::cout << "\t[" << counter++ << "] " << se.departureTime << std::endl; */
    /* } */

    RouteId route{51};
    std::string dump_name = std::string("/tmp/route_") + std::to_string(route.value()) + ".geojson";
    std::ofstream geojson_dump(dump_name);
    std::cout << "Dumping to : " << dump_name << std::endl;
    dump_route_stops(geojson_dump, data, route);

    // Qu'est-ce qui est attendu :

    // avec ma compréhension des choses, ce CH correspond à bucket-CH pour le trajet piéton initial/final :
    const std::string& bucketChBasename = "WORKDIR/BUILD_BUCKETCH_OUTPUT/bucketch.graph";
    CH::CH bucketCH(bucketChBasename);

    ShortcutRAPTOR algorithm(data, bucketCH);
    const size_t numberOfVertices = data.transferGraph.numVertices();

    /* // contenu de l'appel : */
    /* const Vertex source = Vertex(rand() % numberOfVertices); */
    /* const Vertex target = Vertex(rand() % numberOfVertices); */
    /* const int departureTime = ((rand() % (16 * 60 * 60))  // quelque part entre 0h et 16h */
    /*                            + (5 * 60 * 60)            // à laquelle on ajoute 5h */
    /*                            // donc au final = un temps aléatoire entre 5h00 et 21h00 */
    /*                            ); */
    /* algorithm.run(source, departureTime, target); */

    /* std::cout << std::setprecision(10); */

    /* int SOURCE, TARGET, DEPARTURE_TIME; */
    /* for (int i = 0; i < 5; ++i) { */
    /*     std::cout << std::endl << "Please enter SOURCE stop> "; */
    /*     std::cin >> SOURCE; */
    /*     std::cout << std::endl << "Please enter TARGET stop> "; */
    /*     std::cin >> TARGET; */
    /*     std::cout << std::endl << "Please enter DEPARTURE_TIME> "; */
    /*     std::cin >> DEPARTURE_TIME; */

    /*     std::cout << "\n\n" << std::endl; */
    /*     std::cout << "SOURCE  = " << SOURCE << std::endl; */
    /*     std::cout << "TARGET  = " << TARGET << std::endl; */
    /*     std::cout << "DEPTIME = " << DEPARTURE_TIME << std::endl; */
    /*     std::cout << std::endl; */

    /*     std::cout << "Le stop SOURCE " << SOURCE << " a pour data = " << data.stopData[SOURCE] << std::endl; */
    /*     std::cout << "Le stop TARGET " << TARGET << " a pour data = " << data.stopData[TARGET] << std::endl; */
    /*     std::cout << "Running algo..." << std::endl; */
    /*     auto path = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET)); */
    /*     std::cout << "DONE : running algo..." << std::endl; */
    /* } */

    int SOURCE = 435;
    int TARGET = 120;
    int DEPARTURE_TIME = 36000;
    auto path = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET));
    std::ofstream path_stream("/tmp/published/path.geojson");
    dump_journey(path_stream, data, path);

    return 0;
}
