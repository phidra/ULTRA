#include <iostream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include "../Algorithms/RAPTOR/ULTRARAPTOR.h"

#include "../DataStructures/RAPTOR/Data.h"

inline void usage() noexcept {
    std::cout << "Usage: ultra-server  <RAPTOR binary>  <TODO>" << std::endl;
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
    if (argc < 3)
        usage();
    const std::string raptorFile = argv[1];
    const std::string bucketChBasename = argv[2];

    std::cout << "raptorFile       = " << raptorFile << std::endl;
    std::cout << "bucketChBasename = " << raptorFile << std::endl;

    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();

    CH::CH bucketCH(bucketChBasename);
    ShortcutRAPTOR algorithm(data, bucketCH);

    /* int SOURCE = 435; */
    /* int TARGET = 120; */
    int SOURCE = 4350;
    int TARGET = 1200;
    int DEPARTURE_TIME = 36000;
    auto legs = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET));
    std::cout << std::endl;
    std::cout << "LEGS" << std::endl;
    for (auto leg : legs) {
        std::cout << leg.as_string() << std::endl;
    }
    std::cout << std::endl;
    /* std::ofstream path_stream("/tmp/published/path.geojson"); */
    /* dump_journey(path_stream, data, path); */

    return 0;
}
