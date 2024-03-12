#include "Berth.hpp"

std::function<bool(index_t)> Berth::wanted = nullptr;
Berth Berth::virtual_berth{no_index,Position::npos, 0, -1, true};
Berths Berths::berths;