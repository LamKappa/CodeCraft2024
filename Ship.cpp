#include "Ship.hpp"

Ships Ships::ships;
Ship::Mission Ship::Mission::waiting {WAITING, nullptr, no_index, 0};