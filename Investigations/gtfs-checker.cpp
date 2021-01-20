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

void use_parsed_sample(ad::cppgtfs::gtfs::Feed const& feed) {
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
        if (route.first != route.second->getId()) {
            cout << "ERROR : route id is not identical between : " << route.first << " and " << route.second->getId()
                 << endl;
            exit(2);
        }
        cout << "\t shortname = " << route.second->getShortName() << endl;
        cout << "\t longname = " << route.second->getLongName() << endl;
    }

    // STEP 3 = itérer sur les trips d'une route :
    // En fait, dans le format GTFS, il n'y a pas directement de notion de "trip d'une route"
    // Il faut reconstruire la notion, en filtrant les tips associés à une même route.

    // STEP 4 = récupérer un trip/route/stop en particulier :

    // ROUTE :
    string route_id = "STBA";
    auto route_ptr = feed.getRoutes().get(route_id);
    if (route_ptr == 0) {
        cout << "ERROR : route_ptr is 0" << endl;
        exit(1);
    }
    auto& route = *route_ptr;
    cout << "La route '" << route_id << "' a pour longname : " << route.getLongName() << endl;
    RAPTOR::Route parsedRoute{route.getLongName(), route.getType()};
    cout << "PARSED ROUTE = " << parsedRoute << endl;

    // STOP
    string stop_id = "BEATTY_AIRPORT";
    auto stop_ptr = feed.getStops().get(stop_id);
    if (stop_ptr == 0) {
        cout << "ERROR : stop_ptr is 0" << endl;
        exit(1);
    }
    auto& stop = *stop_ptr;
    cout << "Le stop '" << stop_id << "' a pour nom : " << stop.getName();
    cout << " et pour coordonées (" << stop.getLat() << ";" << stop.getLng() << ")" << endl;

    RAPTOR::Stop parsedStop{
        stop.getName(), Geometry::Point{Construct::LatLongTag{}, stop.getLat(), stop.getLng()},
        0  // no min_transfer_time in GTFS
    };
    cout << "PARSED STOP = " << parsedStop << endl;

    // TRIP :
    string trip_id = "BFC1";
    auto trip_ptr = feed.getTrips().get(trip_id);
    if (trip_ptr == 0) {
        cout << "ERROR : trip_ptr is 0" << endl;
        exit(1);
    }
    auto& trip = *(trip_ptr);
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
}

void check_if_all_trips_of_the_same_route_have_similar_stops(ad::cppgtfs::gtfs::Feed const& feed) {
    // l'objectif était de "confirmer" que les trips d'une même route avaient exactement le même set de stops
    // spoiler alert : au moins sur Bordeaux, ça n'est pas le cas
    // (c'est plutôt logique : sur Bordeaux, il n'y a que 4 routes : une par ligne de tram)

    // L'idée va être de regrouper les sets de stops de tous les trips d'une même route dans une map.
    // On s'arrête dès qu'on rencontre, pour une route A, deux de ses trips qui n'ont pas le même set de stops.

    map<string, pair<string, vector<string>>> routeToStops;  // value = {trip_id, trip_stops}

    cout << endl;
    cout << "There are : " << feed.getTrips().size() << " trips" << endl;
    for (auto& trip_pair : feed.getTrips()) {
        auto[trip_id, trip_ptr] = trip_pair;  // yummy, structured-bindings
        auto& trip = *(trip_ptr);
        auto route = *(trip.getRoute());
        // cout << "Trip [" << trip_id << "] is an instance of route : " << route.getLongName() << endl;
        auto route_id = route.getId();

        // on construit un vector avec les stop du trip :
        vector<string> this_trip_stops(trip.getStopTimes().size());
        for (auto const& stoptime : trip.getStopTimes()) {
            cout << "StopTime n°" << stoptime.getSeq() << endl;
            auto stop = *(stoptime.getStop());
            this_trip_stops[stoptime.getSeq() - 1] = stop.getId();
        }

        // si la route n'a encore jamais été rencontrée, ce vector de stops est sa référence :
        if (routeToStops.find(route_id) == routeToStops.end()) {
            routeToStops[route_id] = {trip_id, this_trip_stops};
        }

        // sinon, on compare ce vector de stops avec celui de la route associée au trip :
        else {
            auto[reference_trip, reference_vector] = routeToStops[route_id];
            cout << "REFERENCE :" << endl;
            dump_trip_stops(cout, feed, reference_vector);
            cout << endl;
            cout << endl;
            cout << "CEUX DE CE TRIP :" << endl;
            dump_trip_stops(cout, feed, this_trip_stops);
            cout << endl;
            cout << endl;
            if (reference_vector != this_trip_stops) {
                cout << "ERROR : vectors différents !" << endl;
                cout << "route_id = " << route_id << endl;
                cout << "reference trip of route = " << reference_trip << endl;
                cout << "trip_id = " << trip_id << endl;
                cout << "Le vector de référence a " << reference_vector.size() << " stops" << endl;
                cout << "Le vector du trip a " << this_trip_stops.size() << " stops" << endl;
                /* cout << "REFERENCE :" << endl; */
                /* cout << setprecision(10); */
                /* for (auto stop_id_str: reference_vector) { */
                /*     auto stop_ptr = feed.getStops().get(stop_id_str); */
                /*     if (stop_ptr == 0) { cout << "ERROR : stop_ptr is 0" << endl; exit(1); } */
                /*     auto& stop = *stop_ptr; */
                /*     cout << "\t " << stop_id_str << "   -> " << stop.getLat() << "|" << stop.getLng() << endl; */
                /* } */
                /* cout << endl; */
                /* cout << "CEUX DE CE TRIP :" << endl; */
                /* for (auto stop_id_str: this_trip_stops) { */
                /*     auto stop_ptr = feed.getStops().get(stop_id_str); */
                /*     if (stop_ptr == 0) { cout << "ERROR : stop_ptr is 0" << endl; exit(1); } */
                /*     auto& stop = *stop_ptr; */
                /*     cout << "\t " << stop_id_str << "   -> " << stop.getLat() << "|" << stop.getLng() << endl; */
                /* } */
                /* cout << endl; */
                exit(2);
            }
        }
    }
}

string get_stopset_id(ad::cppgtfs::gtfs::Trip const& trip) {
    if (trip.getStopTimes().size() < 2) {
        cout << "ERROR : stopset is too small (" << trip.getStopTimes().size() << ")" << endl;
        exit(3);
    }

    string stopset_id{};

    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        stopset_id.append(stop.getId());
        stopset_id.append("+");
    }

    // remove final '+' :
    return stopset_id.substr(0, stopset_id.size() - 1);
}

void partition_trips_in_routes(ad::cppgtfs::gtfs::Feed const& feed) {
    // cette fonction partitionne les trips en différentes routes "virtuelles" (appelées "stopset" ci-dessous)
    // cette partition est basée sur les stops : deux trips auront la même route virtuelle s'ils ont exactement les
    // mêmes stops
    // les routes sont dites virtuelles car elles sont différentes des routes définies dans le GTFS
    // (en effet, dans le GTFS, les différents trips d'une même route n'ont pas nécessairement les mêmes stops)

    // ce que je veux faire :
    //      itérer sur les trips
    //      éventuellement, créer une clé à partir d'un set de stops (e.g. la concaténation des stop_ids)
    //      regrouper les trips par cette clé (et ne pas oublier d'associer également la route originale)
    //      in fine, lister :
    //          - vérifier que toutes les routes "réelles" au sein d'une même route virtuelle sont identiques
    //          - lister le nombre de routes virtuelles (au total, et par route réelle)
    //          - lister le nombre de trips par route virtuelle

    cout << endl;
    cout << "There are : " << feed.getTrips().size() << " trips" << endl;

    // stocke pour chaque set de stops les trips associés :
    map<string, unordered_set<string>> stopsetToTrips;

    for (auto& trip_pair : feed.getTrips()) {
        auto[trip_id, trip_ptr] = trip_pair;  // yummy, structured-bindings
        auto& trip = *(trip_ptr);
        auto real_route = *(trip.getRoute());
        // cout << "Trip [" << trip_id << "] is an instance of real_route : " << real_route.getLongName() << endl;
        auto route_id = real_route.getId();

        string stopset_id = get_stopset_id(trip);

        // si le stopset n'a encore jamais été rencontré, on créé un nouveau set :
        if (stopsetToTrips.find(stopset_id) == stopsetToTrips.end()) {
            stopsetToTrips[stopset_id] = {trip_id};
        }
        // sinon, on ajoute au set le trip :
        else {
            auto& stopset_trips = stopsetToTrips[stopset_id];
            stopset_trips.emplace(trip_id);
        }
    }

    // à ce stade, quels sont les stopset et les trips :
    cout << endl;
    for (auto[stopset_id, trips] : stopsetToTrips) {
        cout << "STOPSET = " << stopset_id << endl;
        for (auto trip : trips) {
            cout << "\t TRIP = " << trip << endl;
        }
    }
    cout << endl;

    cout << "Nombre de stopset_id = " << stopsetToTrips.size() << endl;
    cout << "Nombre de trips par stopset :" << endl;
    for (auto[stopset_id, trips] : stopsetToTrips) {
        cout << "STOPSET = " << stopset_id << "       -> " << trips.size() << endl;
    }
    cout << endl;

    // vérification que tous les trips d'un stopset donné ont bien la même route (réelle) :
    for (auto[stopset_id, trips] : stopsetToTrips) {
        unordered_set<string> route_ids;
        for (auto trip_id : trips) {
            auto trip_ptr = feed.getTrips().get(trip_id);
            if (trip_ptr == 0) {
                cout << "ERROR : trip_ptr is 0" << endl;
                exit(1);
            }
            auto& trip = *(trip_ptr);
            auto real_route = *(trip.getRoute());
            auto real_route_id = real_route.getId();
            route_ids.emplace(real_route_id);
        }
        /* cout << "STOPSET = " << stopset_id << "      has " << route_ids.size() */
        /*      << " routes, first one is : " << *(route_ids.begin()) << endl; */
        if (route_ids.size() != 1) {
            cout << "ERROR : last stopset has more/less than 1 real route !" << endl;
            exit(4);
        }
    }
    cout << "OK" << endl;
    cout << endl;
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    const std::string gtfs_folder = argv[1];

    cout << "Parsing GTFS from folder : " << gtfs_folder << endl;

    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    // use_parsed_sample(feed);
    // check_if_all_trips_of_the_same_route_have_similar_stops(feed);
    partition_trips_in_routes(feed);

    return 0;
}
