#pragma once
#ifndef CODECRAFTSDK_ROBOT_HPP
#define CODECRAFTSDK_ROBOT_HPP

#include <array>
#include <future>
#include <istream>

#include "Config.h"
#include "Mission.hpp"
#include "Position.hpp"

struct Robot {
    Position pos;
    Mission mission;
    bool goods{};
    bool status{};

    Robot() = default;

    friend auto &operator>>(std::istream &in, Robot &r) {
        return in >> r.goods >> r.pos >> r.status;
    }
};

struct Robots : public std::array<Robot, ROBOT_NUM> {
    using array::array;
    static Robots robots;
    Robots() = default;

    std::future<void> resolve() {
        return std::async(std::launch::async, [this] {
            for(auto &robot: robots) {
                if(!robot.mission.complete()) { continue; }
            }
        });
    }
};

#endif//CODECRAFTSDK_ROBOT_HPP
