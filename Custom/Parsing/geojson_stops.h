#pragma once

#include <iostream>

#include "../stopmap.h"

namespace myserver {

StopMap load_stopfile(std::istream& stopfile_stream);

void display(StopMap const& stops, std::ostream& out);

}
