#pragma once

#include <vector>

#include <rapidjson/document.h>

#include "../legs.h"
#include "../stopmap.h"

namespace myserver {

rapidjson::Value stop_to_geojson(std::string const& stopId, StopMap const&, rapidjson::Document::AllocatorType&);
rapidjson::Value stop_to_coordinates(std::string const& stopId, StopMap const&, rapidjson::Document::AllocatorType&);

rapidjson::Value leg_to_json(Leg const& leg, StopMap const&, rapidjson::Document::AllocatorType& a);
rapidjson::Value leg_to_geojson_polyline(Leg const& leg, StopMap const& stop2loc, rapidjson::Document::AllocatorType&);

rapidjson::Value legs_to_json(std::vector<Leg> const& legs, StopMap const& stop2loc, rapidjson::Document::AllocatorType&);
rapidjson::Value legs_to_geojson(std::vector<Leg> const& legs, StopMap const& stop2loc, rapidjson::Document::AllocatorType&);

void dump_to_file(rapidjson::Value const& data, std::string filepath);

}
