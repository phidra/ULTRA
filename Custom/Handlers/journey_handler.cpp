#include <chrono>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <httplib.h>

#include "exceptions.h"
#include "journey_handler.h"
#include "../Dumping/json_helper.h"
#include "../legs.h"
#include "../stopmap.h"
#include "../Dumping/duration_helper.h"

using namespace std;

using ALGO = RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>;

struct UnknownStation : public std::exception {
    std::string msg;
    UnknownStation(std::string const& station_id) : msg{std::string("Unknown station '") + station_id + "'"} {}
    inline const char* what() const throw() { return msg.c_str(); }
};


struct JourneyParams {
    JourneyParams() = default;
    JourneyParams(
        string srcid_,
        string srcname_,
        double srclon_,
        double srclat_,
        float src_snap_distance_,
        string dstid_,
        string dstname_,
        double dstlon_,
        double dstlat_,
        float dst_snap_distance_,
        int departure_time_
    ) :
        srcid{srcid_},
        srcname{srcname_},
        srclon{srclon_},
        srclat{srclat_},
        src_snap_distance{src_snap_distance_},
        dstid{dstid_},
        dstname{dstname_},
        dstlon{dstlon_},
        dstlat{dstlat_},
        dst_snap_distance{dst_snap_distance_},
        departure_time{departure_time_}
    {}
    rapidjson::Value as_json(rapidjson::Document::AllocatorType& a) const {
        rapidjson::Value params(rapidjson::kObjectType);
        params.AddMember("srcid", rapidjson::Value().SetString(srcid.c_str(), a), a);
        params.AddMember("srcname", rapidjson::Value().SetString(srcname.c_str(), a), a);
        params.AddMember("srclon", rapidjson::Value(srclon), a);
        params.AddMember("srclat", rapidjson::Value(srclat), a);
        params.AddMember("src_snap_distance", rapidjson::Value(src_snap_distance), a);
        params.AddMember("dstid", rapidjson::Value().SetString(dstid.c_str(), a), a);
        params.AddMember("dstname", rapidjson::Value().SetString(dstname.c_str(), a), a);
        params.AddMember("dstlon", rapidjson::Value(dstlon), a);
        params.AddMember("dstlat", rapidjson::Value(dstlat), a);
        params.AddMember("dst_snap_distance", rapidjson::Value(dst_snap_distance), a);
        params.AddMember("departure_time", rapidjson::Value(departure_time), a);
        params.AddMember("departure_time_str", rapidjson::Value().SetString(format_time(departure_time).c_str(), a), a);
        return params;
    }
    string srcid, srcname;
    double srclon, srclat;
    float src_snap_distance;
    string dstid, dstname;
    double dstlon, dstlat;
    float dst_snap_distance;
    int departure_time;
};

// it is forbidden to provide more than one value for a needed param :
void check_multiple_param(const httplib::Params& params, const string& key) {
    auto count = params.count(key);
    if (count > 1) {
        ostringstream oss;
        oss << "multiple values (" << count << ") for parameter '" << key << "'";
        throw Error400(oss.str());
    }
}

string get_required_param_as_string(const httplib::Params& params, const string& key) {
    // returns the parameter value, as a string

    check_multiple_param(params, key);
    // from now on, there is at most 1 key in params

    auto found = params.find(key);
    if (found == params.end()) {
        ostringstream oss;
        oss << "missing required parameter '" << key << "'";
        throw Error400(oss.str());
    }
    return found->second;
}

int get_required_param_as_int(const httplib::Params& params, const string& key) {
    string param_as_string = get_required_param_as_string(params, key);
    int param_as_int = 0;
    try {
        param_as_int = stoi(param_as_string);
    } catch (exception& e) {
        ostringstream oss;
        oss << "error with parameter '" << key << "' (value=" << param_as_string << ") because  : " << e.what();
        throw Error400(oss.str());
    }
    return param_as_int;
}

JourneyParams parse_stops_params(const httplib::Params& params) {
    // other params are ignored
    string srcid = get_required_param_as_string(params, "srcid");
    string dstid = get_required_param_as_string(params, "dstid");
    int departure_time = get_required_param_as_int(params, "departure-time");

    // FIXME : use real stop locations
    return {srcid, "no-name", 0, 0, 0, dstid, "no-name", 0, 0, 0, departure_time};
}

pair<double, double> parse_location(string const& location_str) {
    vector<string> tokens;
    string token;
    istringstream iss(location_str);
    while(getline(iss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.size() != 2) {
        ostringstream oss;
        oss << "unable to parse location '" << location_str << "', unexpected tokens size = " << tokens.size();
        throw Error400(oss.str());
    }

    double longitude, latitude;
    try {
        longitude = stod(tokens[0]);
        latitude = stod(tokens[1]);
    } catch (exception& e) {
        ostringstream oss;
        oss << "unable to parse location '" << location_str << "', can't use stod because : " << e.what();
        throw Error400(oss.str());
    }
    return {longitude, latitude};
}

rapidjson::Document prepare_response(const httplib::Request& req, httplib::Response& res) {
    // postcondition = has an empty "response" object
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();

    doc.AddMember("requested_path", rapidjson::Value().SetString(req.path.c_str(), a), a);

    rapidjson::Value params(rapidjson::kObjectType);
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        params.AddMember(rapidjson::Value().SetString(it->first.c_str(), a),
                         rapidjson::Value().SetString(it->second.c_str(), a), a);
    }
    doc.AddMember("requested_params", params, a);
    doc.AddMember("response", rapidjson::Value().SetObject().Move(), a);

    return doc;
}

void finalize_response(httplib::Response& res, rapidjson::Document& doc, int http_status, string const& error_msg) {
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    bool is_ok = (http_status == 200);
    doc.AddMember("is_ok", is_ok, a);
    doc.AddMember("http_status", http_status, a);
    doc.AddMember("error_msg", rapidjson::Value().SetString(error_msg.c_str(), a), a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    res.set_content(buffer.GetString(), "application/json");
    res.status = http_status;

    cerr << "[" << http_status << "]";
    cerr << "\t" << (is_ok ? "OK" : error_msg) << endl;
}

bool compute_journey(JourneyParams const& jparams, rapidjson::Value& response_field, rapidjson::Document::AllocatorType& a, ALGO& algo, myserver::StopMap const& stops) {
    response_field.AddMember("journey_params", jparams.as_json(a), a);

    decltype(chrono::high_resolution_clock::now()) before;
    int eat = -1;

    vector<myserver::Leg> legs;
    string printed_journey;
    bool is_raptor_ok = false;
    float walkspeed_km_per_hour = 99;
    string raptor_error_msg = "";
    try {
        before = chrono::high_resolution_clock::now();

        // for now, the ids are the rank -> we can convert them directly :
        int SOURCE = std::stoi(jparams.srcid);
        int TARGET = std::stoi(jparams.dstid);
        legs = algo.run(Vertex(SOURCE), jparams.departure_time, Vertex(TARGET));

        // STUBS :
        eat = 42;
        printed_journey = "STUB";
        walkspeed_km_per_hour = 9999;
        is_raptor_ok = true;
    } catch (UnknownStation e) {
        raptor_error_msg = e.what();
    } catch (...) {
        raptor_error_msg = "unknown error";
    }

    auto after = chrono::high_resolution_clock::now();

    auto computing_time_microseconds = chrono::duration_cast<chrono::microseconds>(after - before).count();
    auto journey_duration = eat - jparams.departure_time;

    response_field.AddMember("is_ok", is_raptor_ok, a);
    response_field.AddMember("walkspeed_km_per_hour", walkspeed_km_per_hour, a);
    response_field.AddMember("error_msg", rapidjson::Value().SetString(raptor_error_msg.c_str(), a), a);
    response_field.AddMember("computing_time_microseconds", computing_time_microseconds, a);
    response_field.AddMember("EAT", eat, a);
    response_field.AddMember("EAT_str", rapidjson::Value().SetString(format_time(eat).c_str(), a), a);
    response_field.AddMember("journey_duration", journey_duration, a);
    response_field.AddMember("journey_duration_str", rapidjson::Value().SetString(format_duration(journey_duration).c_str(), a), a);
    response_field.AddMember("printed_journey", rapidjson::Value().SetString(printed_journey.c_str(), a), a);
    response_field.AddMember("legs", legs_to_json(legs, stops, a), a);

    // dumping legs as geojson :
    auto geojson = legs_to_geojson(legs, stops, a);
    response_field.AddMember("geojson", geojson, a);

    return is_raptor_ok;
}

void handle_journey_between_stops(const httplib::Request& req, httplib::Response& res, RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>& algo, myserver::StopMap const& stops) {
    JourneyParams jparams;
    try {
        jparams = parse_stops_params(req.params);
    } catch (Error400& e) {
        rapidjson::Document doc = prepare_response(req, res);
        finalize_response(res, doc, 400, e.what());
        return;
    }

    // if we get here, params are ok :
    rapidjson::Document doc = prepare_response(req, res);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    bool is_raptor_ok = compute_journey(jparams, doc["response"], a, algo, stops);
    if (is_raptor_ok) {
        finalize_response(res, doc, 200, "");
    } else {
        finalize_response(res, doc, 500, "raptor encountered an error");
    }
}
