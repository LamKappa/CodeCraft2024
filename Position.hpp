#pragma once
#ifndef CODECRAFTSDK_POSITION_HPP
#define CODECRAFTSDK_POSITION_HPP

#include <array>
#include <istream>
#include <tuple>

#include "Config.h"

using u8 = u_int8_t;
using u16 = u_int16_t;
struct Position : public std::pair<u8, u8> {
    using pair::pair;
#define x first
#define y second
    friend std::istream &operator>>(std::istream &in, Position &pos) {
        in >> (int&)pos.x >> (int&)pos.y;
        return in ;
    }
    Position(u16 flatten_pos): pair(flatten_pos / N, flatten_pos % N) {}
    operator u16() const {
        return x * N + y;
    }
    auto operator+(const Position &o) {
        return Position{x + o.x, y + o.y};
    }
    [[nodiscard]] auto outside() const {
        return x >= N || y >= N;
    }
#undef x
#undef y
};

const std::array<Position, 4> Move{
        Position{1u, 0u},
        {0u, 1u},
        {-1u, 0u},
        {0u, -1u},
};

#endif//CODECRAFTSDK_POSITION_HPP