#include <iostream>
#include <string>
#include "ad/cppgtfs/Parser.h"

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

    // STEP 1 = connaître le nombre d'éléments :
    cout << "Le feed contient " << feed.getRoutes().size() << " routes" << endl;
    cout << "Le feed contient " << feed.getStops().size() << " stops" << endl;
    cout << "Le feed contient " << feed.getTrips().size() << " trips" << endl;

    // STEP 2 = itérer sur les routes :
    int counter = 0;
    for (auto const& route : feed.getRoutes()) {
        // en itérant sur un container, on récupère une paire [id->object*]
        cout << "Route #" << counter++ << " : " << endl;
        cout << "\t id (from pair.first)  = " << route.first << endl;
        cout << "\t id (from route.getId) = " << route.second->getId() << endl;
        cout << "\t shortname = " << route.second->getShortName() << endl;
        cout << "\t longname = " << route.second->getLongName() << endl;
    }

    // STEP 3 = itérer sur les trips d'une route :
    // En fait, dans le format GTFS, il n'y a pas directement de notion de "trip d'une route"
    // Il faut reconstruire la notion, en filtrant les tips associés à une même route.

    // STEP 4 = récupérer un trip/route/stop en particulier :

    // ROUTE :
    string route_id = "STBA";
    cout << "La route '" << route_id << "' a pour longname : " << feed.getRoutes().get(route_id)->getLongName() << endl;

    // STOP
    string stop_id = "BEATTY_AIRPORT";
    auto& stop = *(feed.getStops().get(stop_id));
    cout << "Le stop '" << stop_id << "' a pour nom : " << stop.getName();
    cout << " et pour coordonées (" << stop.getLat() << ";" << stop.getLng() << ")" << endl;

    // TRIP :
    string trip_id = "BFC1";
    auto& trip = *(feed.getTrips().get(trip_id));
    cout << "Le trip '" << trip_id << "' a pour headsign : " << trip.getHeadsign() << endl;
    cout << "Ce trip a pour stoptimes : " << endl;

    // itérer sur les stops d'un trip se fait en itérant sur ses stoptimes (qui disposent d'une référence sur le stop) :
    for (auto const& stoptime : trip.getStopTimes()) {
        cout << "StopTime n°" << stoptime.getSeq() << endl;
        cout << "\t DEPARTURE = " << stoptime.getDepartureTime().toString() << endl;
        cout << "\t ARRIVAL = " << stoptime.getArrivalTime().toString() << endl;
        cout << "\t HEADSIGN = " << stoptime.getHeadsign() << endl;
        cout << "\t STOP = " << stoptime.getStop()->getName() << endl;
    }

    return 0;
}
