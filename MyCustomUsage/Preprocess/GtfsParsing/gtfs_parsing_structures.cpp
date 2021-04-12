#include "gtfs_parsing_structures.h"

using namespace std;

namespace my::preprocess {

vector<string> RouteLabel::toStopIds() const {
    // from a given routeLabel, this functions builds back the list of its stop's ids :
    vector<string> stops;
    string token;
    istringstream iss(string(*this));
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}


void ParsedRoute::addTrip(OrderableTripId const& tripId, ad::cppgtfs::gtfs::Trip const& trip)
{
    auto& thisTripEvents = trips[tripId];
    for (auto const& stoptime : trip.getStopTimes()) {
        int arrivalTime = stoptime.getArrivalTime().seconds();
        int departureTime = stoptime.getDepartureTime().seconds();
        thisTripEvents.emplace_back(arrivalTime, departureTime);
    }
}


ParsedStop::ParsedStop(string const& id_, string const & name_, double latitude_, double longitude_) :
        id{id_},
        name{name_},
        latitude{latitude_},
        longitude{longitude_}
    {}


string ParsedStop::as_string() const {
    ostringstream oss;
    oss << "ParsedStop{" << id << ", " << name << ", " << latitude << ", " << longitude << "}";
    return oss.str();
};

}
