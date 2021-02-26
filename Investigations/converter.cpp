#include <iostream>
#include <string>

#include "../Custom/gtfs_to_ultra_binary.h"

using namespace std;

inline void usage() noexcept {
    cout << "Usage: converter <GTFS folder>  <output-file>" << endl;
    exit(1);
}

void display(my::UltraGtfsData const& binaryData);

int main(int argc, char** argv) {
    if (argc < 3)
        usage();

    const string gtfsFolder = argv[1];
    const string outputFile = argv[2];

    cout << "Parsing GTFS from folder    : " << gtfsFolder << endl;
    cout << "Dumping converted binary to : " << outputFile << endl;

    // converting + dumping :
    my::UltraGtfsData binaryData{gtfsFolder};
    cout << "Dumping GTFS binary into : " << outputFile << endl;
    binaryData.dump(outputFile);

    // checking + displaying :
    bool isSerializationIdempotent = binaryData.checkSerializationIdempotence();
    cout << "Is serialization idempotent ? " << isSerializationIdempotent << endl;
    display(binaryData);

    return 0;
}


void display(my::UltraGtfsData const& binaryData) {
    cout << "How many stops ? " << binaryData.stopIds.size() << endl;

    cout << "Here, routeData contains : " << binaryData.routeData.size() << " items." << endl;
    int routeCounter = 0;
    for (auto& route : binaryData.routeData) {
        if (routeCounter++ <= 8) {
            cout << "ROUTE = " << route << endl;
        }
    }

    cout << "Here, stopData contains : " << binaryData.stopData.size() << " items." << endl;
    int stopCounter = 0;
    for (auto& stop : binaryData.stopData) {
        if (stopCounter++ <= 8) {
            cout << "STOP = " << stop << endl;
        }
    }

    cout << "Here, stopIds contains : " << binaryData.stopIds.size() << " items." << endl;
    cout << "Here, firstStopIdOfRoute contains : " << binaryData.firstStopIdOfRoute.size() << " items." << endl;
    int otherRouteCounter = 0;
    for (auto idx : binaryData.firstStopIdOfRoute) {
        if (otherRouteCounter++ <= 8) {
            cout << "First stop id of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopIdOfRoute is " << binaryData.firstStopIdOfRoute.back() << endl;

    cout << "Here, stopEvents contains : " << binaryData.stopEvents.size() << " items." << endl;
    cout << "Here, firstStopEventOfRoute contains : " << binaryData.firstStopEventOfRoute.size() << " items." << endl;
    int yetAnotherRouteCounter = 0;
    for (auto idx : binaryData.firstStopEventOfRoute) {
        if (yetAnotherRouteCounter++ <= 8) {
            cout << "First stop event of this route = " << idx << endl;
        }
    }
    cout << "Last item of firstStopEventOfRoute is " << binaryData.firstStopEventOfRoute.back() << endl;

    cout << "Here, routeSegments contains : " << binaryData.routeSegments.size() << " items." << endl;
    cout << "Here, firstRouteSegmentOfStop contains : " << binaryData.firstRouteSegmentOfStop.size() << " items." << endl;
    int lastRouteCounter = 0;
    for (auto idx : binaryData.firstRouteSegmentOfStop) {
        if (lastRouteCounter++ <= 8) {
            cout << "First route segment of this stop = " << idx << endl;
        }
    }
    cout << "Last item of firstRouteSegmentOfStop is " << binaryData.firstRouteSegmentOfStop.back() << endl;

}
