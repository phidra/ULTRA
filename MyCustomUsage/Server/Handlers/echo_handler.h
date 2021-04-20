#pragma once

namespace httplib {
struct Request;
struct Response;
}  // namespace httplib

namespace myserver {

void handle_echo(const httplib::Request&, httplib::Response&);

}
