#include "Berth.hpp"


std::function<void(index_t,std::function<void()>)> Berth::wanted = nullptr;
Berths Berths::berths;