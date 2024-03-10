#pragma once
#ifndef CODECRAFTSDK_ROBOT_HPP
#define CODECRAFTSDK_ROBOT_HPP

#include <array>
#include <istream>

#include "Config.h"
#include "Position.hpp"

struct Robot {
    Position pos;
    int goods{};
    int status{};

    Robot() = default;

    friend auto &operator>>(std::istream &in, Robot &r) {
        return in >> r.goods >> r.pos >> r.status;
    }
};

struct Robots : public std::array<Robot, ROBOT_NUM> {
    using array::array;
    static Robots robots;
    Robots() = default;
};

#endif//CODECRAFTSDK_ROBOT_HPP
