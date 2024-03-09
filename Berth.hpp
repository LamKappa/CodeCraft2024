#pragma once
#ifndef CODECRAFTSDK_BERTH_HPP
#define CODECRAFTSDK_BERTH_HPP

#include <array>
#include <istream>
#include <tuple>

#include "Config.h"
#include "Position.hpp"

struct Berth {
    int id;
    Position pos;
    int transport_time;
    int loading_speed;

    Berth() = default;

    friend auto &operator>>(std::istream &in, Berth &b) {
        return in >> b.id >> b.pos >> b.transport_time >> b.loading_speed;
    }
};

struct Berths : public std::array<Berth, BERTH_NUM> {
    using array::array;
    static Berths berths;
    Berths() = default;
};

#endif//CODECRAFTSDK_BERTH_HPP
