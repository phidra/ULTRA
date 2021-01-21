#include <iostream>
#include <string>
#include <numeric>
#include <unordered_set>

#include "ad/cppgtfs/Parser.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"
#include "../Custom/gtfs.h"

using namespace std;

inline void usage() noexcept {
    std::cout << "Usage: gtfs-checker <GTFS folder>" << std::endl;
    exit(1);
}

rapidjson::Value stop_to_coordinates(ad::cppgtfs::gtfs::Stop const& stop, rapidjson::Document::AllocatorType& a) {
    rapidjson::Value json_location(rapidjson::kArrayType);
    json_location.PushBack(rapidjson::Value(stop.getLng()), a);
    json_location.PushBack(rapidjson::Value(stop.getLat()), a);
    return json_location;
}

void dump_trip_stops(std::ostream& out, ad::cppgtfs::gtfs::Feed const& feed, vector<string> const& stop_ids) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    // coordinates :
    rapidjson::Value coordinates(rapidjson::kArrayType);
    for (auto stop_id_str : stop_ids) {
        auto stop_ptr = feed.getStops().get(stop_id_str);
        if (stop_ptr == 0) {
            cout << "ERROR : stop_ptr is 0" << endl;
            exit(1);
        }
        auto& stop = *stop_ptr;
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

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    const std::string gtfs_folder = argv[1];

    cout << "Parsing GTFS from folder : " << gtfs_folder << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    // check_if_all_trips_of_the_same_route_have_similar_stops(feed);
    auto[stopsetToTrips, routesToStopsets] = my::partition_trips_in_stopsets(feed);
    my::assert_identical_stopset_routes(feed, stopsetToTrips);

    // Les trips d'une même route ont-ils tous le même stopset ?
    // a.k.a vérifier s'il n'y a qu'un seul stopset par route (spoiler alert : sur Bordeaux, non)
    for (auto const & [ route, stopsets ] : routesToStopsets) {
        cout << "route [" << route << "] has " << stopsets.size() << " different stopsets." << endl;
    }

    return 0;
}
