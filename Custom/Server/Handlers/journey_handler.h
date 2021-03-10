#pragma once

#include "../../../Algorithms/RAPTOR/ULTRARAPTOR.h"
#include "../stopmap.h"

namespace httplib {
    struct Request;
    struct Response;
}

void handle_journey_between_stops(const httplib::Request&, httplib::Response&, RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>&, myserver::StopMap const&);
void handle_journey_between_locations(const httplib::Request&, httplib::Response&, RAPTOR::ULTRARAPTOR<RAPTOR::NoDebugger>&, myserver::StopMap const&);
