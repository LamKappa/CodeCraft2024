#include "Ship.hpp"

std::set<char> Ship::SHIP_BLOCK_SYM{
        MAP_SYMBOLS::SEA,
        MAP_SYMBOLS::SEA_MULTI,
        MAP_SYMBOLS::SHIP,
        MAP_SYMBOLS::SEA_GROUND,
        MAP_SYMBOLS::SEA_GROUND_MULTI,
        MAP_SYMBOLS::COMMIT,
        MAP_SYMBOLS::BERTH,
        MAP_SYMBOLS::BERTH_AROUND,
};

std::set<char> Ship::SHIP_MULTI_SYM{
        MAP_SYMBOLS::SEA_MULTI,
        MAP_SYMBOLS::SHIP,
        MAP_SYMBOLS::SEA_GROUND_MULTI,
        MAP_SYMBOLS::COMMIT,
        MAP_SYMBOLS::BERTH,
        MAP_SYMBOLS::BERTH_AROUND,
};

Ships Ships::ships;
