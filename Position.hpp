#pragma once
#ifndef CODECRAFTSDK_POSITION_HPP
#define CODECRAFTSDK_POSITION_HPP

#include <array>
#include <iostream>
#include <map>
#include <tuple>

#include "Config.h"

using Flatten_Position = u16;
struct Position : public std::pair<u8, u8> {
    using pair::pair;
    static const Position npos;
#define x first
#define y second
    friend std::istream &operator>>(std::istream &in, Position &pos) {
        return in >> (int &) pos.x >> (int &) pos.y;
    }
    friend std::ostream &operator<<(std::ostream &out, Position &pos) {
        return out << "(" << (int) pos.x << "," << (int) pos.y << ")";
    }
    Position(Flatten_Position flatten_pos): pair(flatten_pos / N, flatten_pos % N) {}
    operator Flatten_Position() const {
        return x * N + y;
    }
    auto operator+(const Position &o) {
        return Position{x + o.x, y + o.y};
    }
    auto operator+=(const Position &o) {
        return *this = *this + o;
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

using Direction = Position;

#define RIGHT ((Direction)Move[0])
#define LEFT ((Direction)Move[1])
#define UP ((Direction)Move[2])
#define DOWN ((Direction)Move[3])

inline Direction next(Direction dir){
    if(dir == RIGHT){
        return DOWN;
    }else if(dir == LEFT){
        return UP;
    }else if(dir == UP){
        return RIGHT;
    }else if(dir == DOWN){
        return LEFT;
    }
    return Position::npos;
}

inline Direction prev(Direction dir){
    if(dir == RIGHT){
        return UP;
    }else if(dir == LEFT){
        return DOWN;
    }else if(dir == UP){
        return LEFT;
    }else if(dir == DOWN){
        return RIGHT;
    }
    return Position::npos;
}

const std::map<Position, int> COMMAND{
        {Position::npos, -1},
        {Move[0], 0},
        {Move[1], 1},
        {Move[2], 2},
        {Move[3], 3},
};

#endif//CODECRAFTSDK_POSITION_HPP