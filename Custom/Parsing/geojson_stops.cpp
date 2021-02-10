#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "geojson_stops.h"

using namespace std;

namespace myserver {

struct IllFormattedStopfileException : public std::exception {
    IllFormattedStopfileException(string description) : msg{string("Ill-formatted stopfile : ") + description} {}
    const char* what() const throw() { return msg.c_str(); }
    string msg;
};

static void assert_geojson_format(bool condition, string description) {
    if (!condition)
        throw IllFormattedStopfileException{description};
}

StopMap load_stopfile(istream& stopfile_stream) {
    StopMap stops;

    rapidjson::IStreamWrapper stream_wrapper(stopfile_stream);
    rapidjson::Document doc;
    doc.ParseStream(stream_wrapper);

    assert_geojson_format(doc.IsObject(), "doc is not an object");
    assert_geojson_format(doc.HasMember("features"), "doc has no 'features'");
    auto& features = doc["features"];
    assert_geojson_format(features.IsArray(), "'features' is not an array");

    for (auto& feature: features.GetArray()) {
        assert_geojson_format(feature.IsObject(), "feature is not an object");

        // geometry :
        assert_geojson_format(feature.HasMember("geometry"), "feature has no 'geometry'");
        auto& geometry = feature["geometry"];
        assert_geojson_format(geometry.IsObject(), "geometry is not an object");
        assert_geojson_format(geometry.HasMember("type"), "geometry has no 'type'");
        assert_geojson_format(geometry.HasMember("coordinates"), "geometry has no 'coordinates'");

        auto& geom_type = geometry["type"];
        assert_geojson_format(geom_type.IsString(), "type is not a string");
        assert_geojson_format(string(geom_type.GetString()) == "Point", "type is not a 'Point'");

        auto& coordinates = geometry["coordinates"];
        assert_geojson_format(coordinates.IsArray(), "coordinates is not an Array");

        assert_geojson_format(coordinates.Size() == 2, "coordinates doesn't have 2 values");
        auto& lon = coordinates[0];
        auto& lat = coordinates[1];
        assert_geojson_format(lon.IsDouble(), "lon is not a double");
        assert_geojson_format(lat.IsDouble(), "lat is not a double");

        // properties :
        assert_geojson_format(feature.HasMember("properties"), "feature has no 'properties'");

        auto& properties = feature["properties"];
        assert_geojson_format(properties.IsObject(), "properties is not an object");
        assert_geojson_format(properties.HasMember("stop_id"), "properties has no 'stop_id'");
        assert_geojson_format(properties.HasMember("stop_name"), "properties has no 'stop_name'");

        auto& stop_id = properties["stop_id"];
        assert_geojson_format(stop_id.IsString(), "stop_id is not a string");

        auto& stop_name = properties["stop_name"];
        assert_geojson_format(stop_name.IsString(), "stop_name is not a string");

        stops.emplace(make_pair(stop_id.GetString(), Stop{stop_id.GetString(), stop_name.GetString(), lon.GetDouble(), lat.GetDouble()}));
    }
    return stops;
}

void display(StopMap const& stops, ostream& out) {
    for (auto pair: stops) {
        auto id = pair.first;
        auto stop = pair.second;
        out << "STOP [" << id << "] '" << stop.name << "'  is on point : " << stop.lon << "," << stop.lat << "\n";
    }
    out << endl;
}

}
