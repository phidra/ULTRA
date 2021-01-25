#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include "../DataStructures/RAPTOR/Entities/Stop.h"
#include "../DataStructures/RAPTOR/Entities/Route.h"

using namespace std;

inline void usage() noexcept {
    std::cout << "Usage: converter <GTFS folder>" << std::endl;
    exit(1);
}

vector<RAPTOR::Stop> convert_stopData(ad::cppgtfs::gtfs::Feed const& feed) {
    vector<RAPTOR::Stop> stopData;
    for (auto const & [ stop_id, stop_ptr ] : feed.getStops()) {
        if (stop_ptr == 0) {
            cout << "ERROR : stop_ptr is 0 for id : " << stop_id << endl;
            exit(1);
        }
        auto const& stop = *stop_ptr;
        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData.emplace_back(stop.getName(), location);
    }
    return stopData;
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    const string gtfs_folder = argv[1];

    cout << "Parsing GTFS from folder : " << gtfs_folder << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    cout << "Le feed contient " << feed.getRoutes().size() << " routes" << endl;
    cout << "Le feed contient " << feed.getStops().size() << " stops" << endl;
    cout << "Le feed contient " << feed.getTrips().size() << " trips" << endl;

    vector<RAPTOR::Stop> stopData = convert_stopData(feed);
    ;
    cout << "À ce stade, stopData contient : " << stopData.size() << " items." << endl;
    int counter = 0;
    for (auto& stop : stopData) {
        if (counter++ <= 14) {
            cout << "STOP = " << stop << endl;
        }
    }

    return 0;
}
