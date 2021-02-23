#pragma once

#include <string>
#include <tuple>

#include "../stopmap.h"

void build_index(myserver::StopMap);

std::tuple<std::string, double, double, float> get_closest_stop(double lon, double lat);

