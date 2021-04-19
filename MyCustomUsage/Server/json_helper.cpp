#include <fstream>

#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "json_helper.h"
#include "duration_helper.h"

using namespace std;

namespace myserver {

rapidjson::Value stop_to_geojson(string const& stopId, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    // coordinates :
    rapidjson::Value coordinates = stop_to_coordinates(stopId, stops, a);

    // properties :
    rapidjson::Value properties(rapidjson::kObjectType);
    properties.AddMember("id", rapidjson::Value().SetString(stopId.c_str(), a), a);

    auto stop_name = stopid_to_stopname(stopId, stops, "STOPID NOT IN STOPS");
    properties.AddMember("name", rapidjson::Value().SetString(stop_name.c_str(), a), a);
    auto stop = stops.at(stopId);
    properties.AddMember("lon", rapidjson::Value(stop.lon), a);
    properties.AddMember("lat", rapidjson::Value(stop.lat), a);

    // geometry :
    rapidjson::Value geometry(rapidjson::kObjectType);
    geometry.AddMember("type", "Point", a);
    geometry.AddMember("coordinates", coordinates, a);

    // feature :
    rapidjson::Value feature(rapidjson::kObjectType);
    feature.AddMember("type", "Feature", a);
    feature.AddMember("geometry", geometry, a);
    feature.AddMember("properties", properties, a);
    return feature;
}


rapidjson::Value stop_to_coordinates(string const& stopId, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    // special value [0.0, 0.0] is placeholder for "not found"
    rapidjson::Value json_location(rapidjson::kArrayType);

    auto iterator = stops.find(stopId);
    if (iterator != stops.end()) {
        auto stop = iterator->second;
        json_location.PushBack(rapidjson::Value(stop.lon), a);
        json_location.PushBack(rapidjson::Value(stop.lat), a);
    }
    else {
        json_location.PushBack(rapidjson::Value(0.0), a);
        json_location.PushBack(rapidjson::Value(0.0), a);
    }
    return json_location;
}


rapidjson::Value leg_to_json(Leg const& leg, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    rapidjson::Value json_leg(rapidjson::kObjectType);
    json_leg.AddMember("type", rapidjson::Value().SetString((leg.is_walk ? "walk" : "pt"), a), a);
    json_leg.AddMember("departure_id", rapidjson::Value().SetString(leg.departure_id.c_str(), a), a);
    auto departure_name = stopid_to_stopname(leg.departure_id, stops, "UNKNOWN-NAME");
    json_leg.AddMember("departure_name", rapidjson::Value().SetString(departure_name.c_str(), a), a);
    json_leg.AddMember("departure_location", stop_to_coordinates(leg.departure_id, stops, a), a);
    json_leg.AddMember("arrival_id", rapidjson::Value().SetString(leg.arrival_id.c_str(), a), a);
    auto arrival_name = stopid_to_stopname(leg.arrival_id, stops, "UNKNOWN-NAME");
    json_leg.AddMember("arrival_name", rapidjson::Value().SetString(arrival_name.c_str(), a), a);
    json_leg.AddMember("arrival_location", stop_to_coordinates(leg.arrival_id, stops, a), a);
    json_leg.AddMember("start_time", leg.start_time, a);
    json_leg.AddMember("start_time_str", rapidjson::Value().SetString(my::format_time(leg.start_time).c_str(), a), a);
    json_leg.AddMember("departure_time", leg.departure_time, a);
    json_leg.AddMember("departure_time_str", rapidjson::Value().SetString(my::format_time(leg.departure_time).c_str(), a), a);
    json_leg.AddMember("arrival_time", leg.arrival_time, a);
    json_leg.AddMember("arrival_time_str", rapidjson::Value().SetString(my::format_time(leg.arrival_time).c_str(), a), a);
    json_leg.AddMember("full_duration", leg.get_full_duration(), a);
    json_leg.AddMember("full_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_full_duration()).c_str(), a), a);
    json_leg.AddMember("waiting_duration", leg.get_waiting_duration(), a);
    json_leg.AddMember("waiting_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_waiting_duration()).c_str(), a), a);
    json_leg.AddMember("traveling_duration", leg.get_traveling_duration(), a);
    json_leg.AddMember("traveling_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_traveling_duration()).c_str(), a), a);

    // intermediary stops :
    rapidjson::Value json_stops(rapidjson::kArrayType);
    for (auto stop : leg.stops) {
        json_stops.PushBack(rapidjson::Value().SetString(stop.c_str(), a), a);
    }
    json_leg.AddMember("stops", json_stops, a);
    return json_leg;
}

rapidjson::Value legs_to_json(vector<Leg> const& legs, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    rapidjson::Value json_legs(rapidjson::kArrayType);
    for (auto leg : legs) {
        json_legs.PushBack(leg_to_json(leg, stops, a), a);
    }
    return json_legs;
}

rapidjson::Value leg_to_geojson_polyline(Leg const& leg, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    // coordinates :
    rapidjson::Value coordinates(rapidjson::kArrayType);

    if (leg.is_walk) {
        rapidjson::Value src_coordinates = stop_to_coordinates(leg.departure_id, stops, a);
        rapidjson::Value dst_coordinates = stop_to_coordinates(leg.arrival_id, stops, a);
        coordinates.PushBack(src_coordinates, a);
        coordinates.PushBack(dst_coordinates, a);
    }
    else {
        for (auto stop: leg.stops) {
            rapidjson::Value stop_coordinates = stop_to_coordinates(stop, stops, a);
            coordinates.PushBack(stop_coordinates, a);
        }
    }

    // properties :
    rapidjson::Value properties(rapidjson::kObjectType);
    properties.AddMember("type", rapidjson::Value().SetString((leg.is_walk ? "walk" : "pt"), a), a);
    properties.AddMember("departure_id", rapidjson::Value().SetString(leg.departure_id.c_str(), a), a);
    auto departure_name = stopid_to_stopname(leg.departure_id, stops, "UNKNOWN-NAME");
    properties.AddMember("departure_name", rapidjson::Value().SetString(departure_name.c_str(), a), a);
    properties.AddMember("arrival_id", rapidjson::Value().SetString(leg.arrival_id.c_str(), a), a);
    auto arrival_name = stopid_to_stopname(leg.arrival_id, stops, "UNKNOWN-NAME");
    properties.AddMember("arrival_name", rapidjson::Value().SetString(arrival_name.c_str(), a), a);
    properties.AddMember("start_time", leg.start_time, a);
    properties.AddMember("start_time_str", rapidjson::Value().SetString(my::format_time(leg.start_time).c_str(), a), a);
    properties.AddMember("departure_time", leg.departure_time, a);
    properties.AddMember("departure_time_str", rapidjson::Value().SetString(my::format_time(leg.departure_time).c_str(), a), a);
    properties.AddMember("arrival_time", leg.arrival_time, a);
    properties.AddMember("arrival_time_str", rapidjson::Value().SetString(my::format_time(leg.arrival_time).c_str(), a), a);
    properties.AddMember("full_duration", leg.get_full_duration(), a);
    properties.AddMember("full_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_full_duration()).c_str(), a), a);
    properties.AddMember("waiting_duration", leg.get_waiting_duration(), a);
    properties.AddMember("waiting_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_waiting_duration()).c_str(), a), a);
    properties.AddMember("traveling_duration", leg.get_traveling_duration(), a);
    properties.AddMember("traveling_duration_str", rapidjson::Value().SetString(my::format_duration(leg.get_traveling_duration()).c_str(), a), a);

    // geometry :
    rapidjson::Value geometry(rapidjson::kObjectType);
    geometry.AddMember("type", "LineString", a);
    geometry.AddMember("coordinates", coordinates, a);

    // feature :
    rapidjson::Value feature(rapidjson::kObjectType);
    feature.AddMember("type", "Feature", a);
    feature.AddMember("geometry", geometry, a);
    feature.AddMember("properties", properties, a);
    return feature;
}

rapidjson::Value legs_to_geojson(vector<Leg> const& legs, StopMap const& stops, rapidjson::Document::AllocatorType& a) {
    // {
    //   "type": "FeatureCollection",
    //   "features": [
    //     {
    //       "type": "Feature",
    //       "properties": {},
    //       "geometry": {
    //         "type": "LineString",
    //         "coordinates": [
    //           [2.72735595703125, 47.924624978768314],
    //           [2.7383422851562496, 47.954064687296885],
    //           [2.7836608886718746, 47.95498440806741],
    //           [2.7850341796875, 47.97889140226657]
    //         ]
    //       }
    //     },
    //     {
    //       "type": "Feature",
    //       "properties": {},
    //       "geometry": {
    //         "type": "LineString",
    //         "coordinates": [
    //           [2.78228759765625, 47.989002568678686],
    //           [2.7836608886718746, 48.011975126709956],
    //           [2.8289794921874996, 48.011056420797836]
    //         ]
    //       }
    //     }
    //   ]
    // }

    rapidjson::Value result(rapidjson::kObjectType);
    result.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);
    for (auto& leg : legs) {
        rapidjson::Value polyline = leg_to_geojson_polyline(leg, stops, a);
		rapidjson::Value src_point = stop_to_geojson(leg.departure_id, stops, a);
		rapidjson::Value dst_point = stop_to_geojson(leg.arrival_id, stops, a);
        features.PushBack(src_point, a);
        features.PushBack(polyline, a);
        features.PushBack(dst_point, a);
    }

    result.AddMember("features", features, a);
    return result;
}

void dump_to_file(rapidjson::Value const& data, string filepath) {
    ofstream out(filepath);
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    data.Accept(writer);
}

}
