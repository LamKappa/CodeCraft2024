#pragma once
#ifndef CODECRAFTSDK_BOAT_HPP
#define CODECRAFTSDK_BOAT_HPP

#include <array>
#include <istream>

#include "Config.h"

struct Boat {
    int id{};
    int load = 0;
    int status{}, pos{};
    static int CAPACITY;

    Boat() = default;

    friend auto &operator>>(std::istream &in, Boat &b) {
        return in >> b.status >> b.pos;
    }
};

struct Boats : public std::array<Boat, BOAT_NUM> {
    using array::array;
    static Boats boats;
    Boats() = default;
};

#endif//CODECRAFTSDK_BOAT_HPP
