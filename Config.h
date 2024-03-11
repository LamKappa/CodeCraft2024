#pragma once
#ifndef CODECRAFTSDK_CONFIG_H
#define CODECRAFTSDK_CONFIG_H

#include <random>

using u8 = u_int8_t;
using u16 = u_int16_t;
using u32 = u_int32_t;
using u64 = u_int64_t;

using index_t = u16;
constexpr index_t no_index = -1;

constexpr int N = 200;
constexpr int ROBOT_NUM = 10;
constexpr int BERTH_NUM = 10;
constexpr int SHIP_NUM = 5;
constexpr int MAX_FRAME = 15000;

extern int stamp, money;
extern std::mt19937 eng;
extern int SHIP_CAPACITY;

#endif//CODECRAFTSDK_CONFIG_H
