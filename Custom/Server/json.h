#pragma once

#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

rapidjson::Value stop_to_coordinates(RAPTOR::Stop const& stop, rapidjson::Document::AllocatorType& a) {
    rapidjson::Value json_location(rapidjson::kArrayType);
    json_location.PushBack(rapidjson::Value(stop.coordinates.longitude), a);
    json_location.PushBack(rapidjson::Value(stop.coordinates.latitude), a);
    return json_location;
}

void dump_journey(std::ostream& out, RAPTOR::Data& data, std::vector<::StopId> path) {
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
