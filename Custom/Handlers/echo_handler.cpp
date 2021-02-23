#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <httplib.h>

#include "echo_handler.h"

using namespace std;

namespace myserver {

void handle_echo(const httplib::Request& req, httplib::Response& res) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("method", rapidjson::Value().SetString(req.method.c_str(), a), a);
    doc.AddMember("version", rapidjson::Value().SetString(req.version.c_str(), a), a);
    doc.AddMember("path", rapidjson::Value().SetString(req.path.c_str(), a), a);

    // params :
    rapidjson::Value params(rapidjson::kObjectType);
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        // string usage seems to be very tedious in RapidJson ... ?
        params.AddMember(rapidjson::Value().SetString(it->first.c_str(), a),
                         rapidjson::Value().SetString(it->second.c_str(), a), a);
    }
    doc.AddMember("params", params, a);

    // headers :
    rapidjson::Value headers(rapidjson::kObjectType);
    for (auto it = req.headers.begin(); it != req.headers.end(); ++it) {
        headers.AddMember(rapidjson::Value().SetString(it->first.c_str(), a),
                          rapidjson::Value().SetString(it->second.c_str(), a), a);
    }
    doc.AddMember("headers", headers, a);

    doc.Accept(writer);

    res.set_content(buffer.GetString(), "application/json");
}

}
