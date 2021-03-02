#include <iostream>
#include <string>
#include <filesystem>

#include <httplib.h>

#include "../Algorithms/RAPTOR/ULTRARAPTOR.h"

#include "../DataStructures/RAPTOR/Data.h"
#include "../Custom/json.h"
#include "../Custom/Parsing/geojson_stops.h"
#include "../Custom/Parsing/gtfs_stops.h"
#include "../Custom/Dumping/json_helper.h"
#include "../Custom/Handlers/echo_handler.h"
#include "../Custom/Handlers/journey_handler.h"
#include "../Custom/Snapping/snapping.h"

inline void usage() noexcept {
    std::cout << "Usage: ultra-server  <port>  <RAPTOR binary>  <bucketCH-basename>  <stopfile>\n";
    std::cout << "\n";
    std::cout << "This is a BLOCKING server -> do NOT use in anything remotely close to production !\n";
    std::cout << std::endl;
    exit(0);
}

using ShortcutRAPTOR = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

int main(int argc, char** argv) {
    if (argc < 5)
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
    const std::string stopfile_path = argv[4];

    std::cout << "raptorFile            = " << raptorFile << std::endl;
    std::cout << "bucketChBasename      = " << bucketChBasename << std::endl;
    std::cout << "stopfile_path         = " << stopfile_path << std::endl;

    RAPTOR::Data data = RAPTOR::Data::FromBinary(raptorFile);
    data.useImplicitDepartureBufferTimes();
    data.printInfo();

    CH::CH bucketCH(bucketChBasename);
    ShortcutRAPTOR algorithm(data, bucketCH);

    std::ifstream stopfile_stream(stopfile_path);
    auto stopmap = myserver::load_stopfile(stopfile_stream);
    std::cout << "Number of loaded stops = " << stopmap.size() << std::endl;

    /* /1* int SOURCE = 435; *1/ */
    /* /1* int TARGET = 120; *1/ */
    /* u_int32_t SOURCE = 4350; */
    /* u_int32_t TARGET = 1200; */
    /* int DEPARTURE_TIME = 36000; */

    /* if (!data.isStop(Vertex{SOURCE}) || !data.isStop(Vertex{TARGET})) { */
    /*     std::cout << "For now, we can only handle journeys between stops :" << std::endl; */
    /*     std::cout << "Is SOURCE a stop ? " << data.isStop(Vertex{SOURCE}) << std::endl; */
    /*     std::cout << "Is TARGET a stop ? " << data.isStop(Vertex{TARGET}) << std::endl; */
    /*     return 1; */
    /* } */

    /* auto legs = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET)); */
    /* std::cout << std::endl; */
    /* std::cout << "JOURNEY = " << SOURCE << "   -->   " << TARGET << std::endl; */
    /* std::cout << "LEGS" << std::endl; */
    /* for (auto leg : legs) { */
    /*     std::cout << leg.as_string() << std::endl; */
    /* } */
    /* std::cout << std::endl; */

    // ideally, we'd like to have a stopmap with stop infos (name, id, ...)
    // for now, we build a stopmap from the transferGraph, which has very few infos on stops :
    // this "coarse" stopmap only has rank and coordinates of the stops.
    auto numStops = data.numberOfStops();
    std::cout << "How many stops in the transferGraph : " << numStops << std::endl;
    myserver::StopMap coarse_stopmap;
    for (int stopRank = 0; stopRank < numStops; ++stopRank) {
        Geometry::Point coords = data.transferGraph.get(Coordinates, Vertex(stopRank));

        // as we have no further info on stops in ULTRA data, for now, id and name are identical to the rank :
        std::string id = std::to_string(stopRank);
        std::string name = std::to_string(stopRank);
        coarse_stopmap.emplace(make_pair(id, myserver::Stop{id, name, coords.longitude, coords.latitude}));
    }
    std::cout << std::endl;
    build_index(coarse_stopmap);

    std::cout << "How many stops in the coarse stopmap : " << coarse_stopmap.size() << std::endl;
    std::cout << std::endl;

    /* // FIXME : temporarily dumping legs as geojson : */
    /* rapidjson::Document doc(rapidjson::kObjectType); */
    /* rapidjson::Document::AllocatorType& a = doc.GetAllocator(); */
    /* auto geojson = myserver::legs_to_geojson(legs, coarse_stopmap, a); */
    /* myserver::dump_to_file(geojson, "/tmp/journey.geojson"); */


    httplib::Server svr;

    // echo :
    svr.Get("/echo", myserver::handle_echo);

    // journey between stops :
    auto f1 = [&algorithm, &coarse_stopmap](const httplib::Request& req, httplib::Response& res) {
        handle_journey_between_stops(req, res, algorithm, coarse_stopmap);
    };
    svr.Get("/journey_between_stops", f1);

    // journey between locations :
    auto f2 = [&algorithm, &coarse_stopmap](const httplib::Request& req, httplib::Response& res) {
        handle_journey_between_locations(req, res, algorithm, coarse_stopmap);
    };
    svr.Get("/journey_between_locations", f2);

    // Serving a viewer, which files are located in "src/Static/viewer". This assumes that :
    //      program binary is in :  src/_build/bin/
    //      viewer folder is in :   src/Static/viewer/
    std::filesystem::path program_path{argv[0]};
    auto src_path = program_path.parent_path().parent_path().parent_path();
    auto viewer_path = src_path / "Static" / "viewer";
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
