#include "Ship.hpp"

Ships Ships::ships;
std::map<index_t, std::reference_wrapper<const std::function<void()>>> Ships::waitlist;