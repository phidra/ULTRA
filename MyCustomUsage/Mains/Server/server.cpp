#include <iostream>
#include <string>
#include <filesystem>

#include <httplib.h>

#include "Algorithms/RAPTOR/ULTRARAPTOR.h"
#include "DataStructures/RAPTOR/Data.h"

#include "Server/Snapping/snapping.h"
#include "Server/Handlers/echo_handler.h"
#include "Server/Handlers/journey_handler.h"

using std::cout;
using std::endl;

inline void usage() noexcept {
    std::cout << "Usage: ultra-server  <port>  <RAPTOR binary>  <bucketCH-basename>\n";
    std::cout << "\n";
    std::cout << "This is a BLOCKING server -> do NOT use in anything remotely close to production !\n";
    std::cout << std::endl;
    exit(0);
}

using ShortcutRAPTOR = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

int main(int argc, char** argv) {
    if (argc < 4)
        usage();
    int port = 0;
    try {
        port = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "ERROR : unable to parse port '" << argv[1] << "'" << std::endl;
        usage();
        exit(1);
    }
    std::cerr << "Listening to port " << port << std::endl;
    const std::string raptorFile = argv[2];
    const std::string bucketChBasename = argv[3];

    std::cout << "raptorFile            = " << raptorFile << std::endl;
    std::cout << "bucketChBasename      = " << bucketChBasename << std::endl;

    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();

    CH::CH bucketCH(bucketChBasename);
    ShortcutRAPTOR algorithm(data, bucketCH);

    // ideally, we'd like to have a stopmap with detailed stop infos (name, id, ...)
    // for now, we build a stopmap from the transferGraph, which has very few infos on stops :
    // this "coarse" stopmap only has rank and coordinates of the stops.
    // EDIT : actually, we can get at least the name from raptorData.
    auto numStops = data.numberOfStops();
    std::cout << "How many stops in the transferGraph : " << numStops << std::endl;
    myserver::StopMap coarse_stopmap;
    for (int stopRank = 0; stopRank < numStops; ++stopRank) {
        Geometry::Point coords = data.transferGraph.get(Coordinates, Vertex(stopRank));

        // as we have no further info on stops in ULTRA data, for now, id and name are identical to the rank :
        std::string id = std::to_string(stopRank);
        std::string name = data.stopData[stopRank].name;
        coarse_stopmap.emplace(make_pair(id, myserver::Stop{id, name, coords.longitude, coords.latitude}));
    }
    std::cout << std::endl;
    myserver::build_index(coarse_stopmap);

    std::cout << "How many stops in the coarse stopmap : " << coarse_stopmap.size() << std::endl;
    std::cout << std::endl;

    httplib::Server svr;

    // echo :
    svr.Get("/echo", myserver::handle_echo);

    // journey between stops :
    auto f1 = [&algorithm, &coarse_stopmap](const httplib::Request& req, httplib::Response& res) {
        myserver::handle_journey_between_stops(req, res, algorithm, coarse_stopmap);
    };
    svr.Get("/journey_between_stops", f1);

    // journey between locations :
    auto f2 = [&algorithm, &coarse_stopmap](const httplib::Request& req, httplib::Response& res) {
        myserver::handle_journey_between_locations(req, res, algorithm, coarse_stopmap);
    };
    svr.Get("/journey_between_locations", f2);

    std::filesystem::path program_path{argv[0]};
    // Serving a viewer (this depends on a suitable organization of the folders in the repo) :
    auto src_path = program_path.parent_path().parent_path().parent_path();
    auto viewer_path = src_path / "MyCustomUsage" / "Static" / "viewer";
    std::cerr << "Serving viewer from folder : " << viewer_path << std::endl;
    auto ret = svr.set_mount_point("/viewer", viewer_path.string().c_str());
    if (!ret) {
        std::cerr << "ERROR with serving viewer from folder : " << viewer_path << std::endl;
        return 1;
    }

    svr.listen("0.0.0.0", port);
    std::cerr << "Exiting" << std::endl;

    return 0;
}
