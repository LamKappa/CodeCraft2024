#pragma once
#ifndef CODECRAFTSDK_POSITION_HPP
#define CODECRAFTSDK_POSITION_HPP

#include <array>
#include <istream>
#include <tuple>
#include <map>

#include "Config.h"

struct Position : public std::pair<u8, u8> {
    using pair::pair;
    static const Position npos;
#define x first
#define y second
    friend std::istream &operator>>(std::istream &in, Position &pos) {
        in >> (int &) pos.x >> (int &) pos.y;
        return in;
    }
    Position(u16 flatten_pos): pair(flatten_pos / N, flatten_pos % N) {}
    operator u16() const {
        return x * N + y;
    }
    auto operator+(const Position &o) {
        return Position{x + o.x, y + o.y};
    }
    [[nodiscard]] inline auto outside() const {
        return x >= N || y >= N;
    }
#undef x
#undef y
};


constexpr std::array<Position, 4> Move{
        Position{0u, 1u},
        {0u, -1u},
        {-1u, 0u},
        {1u, 0u},
};

const std::map<Position, int> COMMAND{
        {Position::npos, -1},
        {Move[0], 0},
        {Move[1], 1},
        {Move[2], 2},
        {Move[3], 3},
};

#endif//CODECRAFTSDK_POSITION_HPP