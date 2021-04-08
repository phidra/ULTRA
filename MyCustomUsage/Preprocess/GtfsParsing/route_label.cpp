#include "route_label.h"

using namespace std;

namespace my::preprocess {

RouteLabel RouteLabel::fromTrip(ad::cppgtfs::gtfs::Trip const& trip) {
    RouteLabel toReturn;
    // build the label of the trip's route (scientific route, see below).
    // A route label is just the concatenation of its stop's ids :
    //     32+33+34+122+123+125+126
    // precondition : no stopID constains the delimiter '+'

    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
    }

    string routeId{};

#ifndef NDEBUG
    int previousDepartureTime = -1;
#endif

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        routeId.append(stop.getId());
        routeId.append("+");

#ifndef NDEBUG
        // verifying that stop times are properly ordered :
        int currentDepartureTime = stoptime.getDepartureTime().seconds();
        if (currentDepartureTime <= previousDepartureTime) {
            throw runtime_error( "ERROR : stoptimes are not properly ordered !");
        }
        previousDepartureTime = currentDepartureTime;
#endif
    }

    // remove final '+' :
    toReturn.label = routeId.substr(0, routeId.size() - 1);
    return toReturn;
}

vector<std::string> RouteLabel::toStopIds() const {
    // from a given routeLabel, this functions builds back the list of its stop's ids :
    vector<std::string> stops;
    string token;
    istringstream iss(string(*this));
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

}
