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

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    const std::string gtfs_folder = argv[1];

    cout << "Parsing GTFS from folder : " << gtfs_folder << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    cout << "Le feed contient " << feed.getRoutes().size() << " routes" << endl;
    cout << "Le feed contient " << feed.getStops().size() << " stops" << endl;
    cout << "Le feed contient " << feed.getTrips().size() << " trips" << endl;

    std::vector<RAPTOR::Stop> stopData;

    int counter = 0;
    for (auto const & [ stop_id, stop_ptr ] : feed.getStops()) {
        if (stop_ptr == 0) {
            cout << "ERROR : stop_ptr is 0 for id : " << stop_id << endl;
            exit(1);
        }
        auto const& stop = *stop_ptr;
        if (counter++ <= 14) {
            cout << "STOP = " << stop.getId() << endl;
            cout << "\t " << stop.getName() << endl;
            cout << "\t " << stop.getLat() << endl;
            cout << "\t " << stop.getLng() << endl;
        }

        Geometry::Point location{Construct::LatLongTag{}, stop.getLat(), stop.getLng()};
        stopData.emplace_back(stop.getName(), location);
    }

    cout << "À ce stade, stopData contient : " << stopData.size() << " items." << endl;

    return 0;
}
