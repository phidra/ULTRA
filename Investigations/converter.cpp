#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

#include "../Custom/gtfs_to_ultra_binary.h"

using namespace std;

inline void usage() noexcept {
    cout << "Usage: converter <GTFS folder>  <output-file>" << endl;
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 3)
        usage();

    const string gtfs_folder = argv[1];
    const string output_file = argv[2];

    cout << "Parsing GTFS from folder    : " << gtfs_folder << endl;
    cout << "Dumping converted binary to : " << output_file << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    cout << "This feed contains " << feed.getRoutes().size() << " routes" << endl;
    cout << "This feed contains " << feed.getStops().size() << " stops" << endl;
    cout << "This feed contains " << feed.getTrips().size() << " trips" << endl;

    my::do_the_full_preparation(feed, output_file);
    cout << "Dumped GTFS binary into : " << output_file << endl;

    return 0;
}
