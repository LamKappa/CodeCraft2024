#include "Ship.hpp"

std::map<std::pair<index_t, index_t>, std::pair<int, index_t>> Ship::transport_record;
Ship::Mission Ship::Mission::waiting{WAITING, nullptr, no_index, 0};
Ships Ships::ships;
