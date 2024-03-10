#include "Berth.hpp"


std::function<bool(index_t)> Berth::wanted = nullptr;
Berths Berths::berths;