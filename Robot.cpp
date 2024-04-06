#include "Robot.hpp"

Robot::Mission Robot::Mission::idle;
Robots Robots::robots;

const std::set<char> Robots::ROBOT_MULTI_SYM{
        MAP_SYMBOLS::GROUND_MULTI,
        MAP_SYMBOLS::ROBOT,
        MAP_SYMBOLS::SEA_GROUND_MULTI,
        MAP_SYMBOLS::BERTH,
};
