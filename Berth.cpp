#include "Berth.hpp"

Berth Berth::virtual_berth{no_index,Position::npos, Berth::MAX_TRANSPORT_TIME, -1, true};
Berths Berths::berths;