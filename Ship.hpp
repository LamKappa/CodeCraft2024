#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>

#include "Config.h"

struct Ship {
    int id{};
    int load = 0;
    int status{}, pos{};
    static int CAPACITY;

    Ship() = default;

    friend auto &operator>>(std::istream &in, Ship &b) {
        return in >> b.status >> b.pos;
    }
};

struct Ships : public std::array<Ship, BOAT_NUM> {
    using array::array;
    static Ships ships;
    Ships() = default;

    std::future<void> resolve() {
        return std::async(std::launch::async, [this] {

        });
    }
};

#endif//CODECRAFTSDK_SHIP_HPP
