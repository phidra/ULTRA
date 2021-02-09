#include "stopfile.h"

#include "csv.h"

using namespace std;

namespace my {

vector<Stop> parse_stopfile(const char* stopfile, istream& stopfile_stream) {
    // clang-format off
    //
    // BORDEAUX :
    // stop_id , stop_name   , stop_lat  , stop_lon  , stop_desc , zone_id , stop_url , stop_code , location_type , parent_station
    // 3684    , Lauriers    , 44.879107 , -0.517867 ,           , 33249   ,          , LAURI     , 0             , LAURI
    // 3685    , Bois Fleuri , 44.875987 , -0.519344 ,           , 33249   ,          , BFLEA     , 0             , BFLEU
    //
    // IDF :
    // stop_id , stop_code , stop_name    , stop_desc                , stop_lat          , stop_lon          , location_type , parent_station
    // 1166824 ,           , "Olympiades" , "Rue de Tolbiac - 75113" , 48.82694828196076 , 2.367038433387592 , 0             ,
    // 1166825 ,           , "Olympiades" , "Rue de Tolbiac - 75113" , 48.82694828196076 , 2.367038433387592 , 0             ,
    //
    // clang-format on

    vector<Stop> all_stops;

    io::CSVReader<4, io::trim_chars<>, io::double_quote_escape<',', '"'> > in(stopfile, stopfile_stream);
    in.read_header(io::ignore_extra_column, "stop_id", "stop_name", "stop_lat", "stop_lon");
    string stop_id, stop_name;
    double stop_lat, stop_lon;
    while(in.read_row(stop_id, stop_name, stop_lat, stop_lon)) {
        all_stops.emplace_back(stop_lon, stop_lat, stop_id, stop_name);
    }

    return all_stops;
}

}
