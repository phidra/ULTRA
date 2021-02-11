#include <iostream>
#include <string>

#include "../Algorithms/RAPTOR/ULTRARAPTOR.h"

#include "../DataStructures/RAPTOR/Data.h"
#include "../Custom/json.h"
#include "../Custom/Parsing/geojson_stops.h"
#include "../Custom/Parsing/gtfs_stops.h"
#include "../Custom/Dumping/json_helper.h"

inline void usage() noexcept {
    std::cout << "Usage: ultra-server  <RAPTOR binary>  <bucketCH-basename>  <stopfile>" << std::endl;
    exit(0);
}

using ShortcutRAPTOR = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

int main(int argc, char** argv) {
    if (argc < 4)
        usage();
    const std::string raptorFile = argv[1];
    const std::string bucketChBasename = argv[2];
    const std::string stopfile_path = argv[3];

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

    /* int SOURCE = 435; */
    /* int TARGET = 120; */
    u_int32_t SOURCE = 4350;
    u_int32_t TARGET = 1200;
    int DEPARTURE_TIME = 36000;

    if (!data.isStop(Vertex{SOURCE}) || !data.isStop(Vertex{TARGET})) {
        std::cout << "For now, we can only handle journeys between stops :" << std::endl;
        std::cout << "Is SOURCE a stop ? " << data.isStop(Vertex{SOURCE}) << std::endl;
        std::cout << "Is TARGET a stop ? " << data.isStop(Vertex{TARGET}) << std::endl;
        return 1;
    }

    auto legs = algorithm.run(Vertex(SOURCE), DEPARTURE_TIME, Vertex(TARGET));
    std::cout << std::endl;
    std::cout << "JOURNEY = " << SOURCE << "   -->   " << TARGET << std::endl;
    std::cout << "LEGS" << std::endl;
    for (auto leg : legs) {
        std::cout << leg.as_string() << std::endl;
    }
    std::cout << std::endl;

    // ideally, we'd like to have a stopmap with stop infos (name, id, ...)
    // for now, we build a stopmap with stopinfo from the transferGraph :
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

    std::cout << "How many stops in the coarse stopmap : " << coarse_stopmap.size() << std::endl;
    std::cout << std::endl;

    // FIXME : temporarily dumping legs as geojson :
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    auto geojson = myserver::legs_to_geojson(legs, coarse_stopmap, a);
    myserver::dump_to_file(geojson, "/tmp/journey.geojson");

    return 0;
}
