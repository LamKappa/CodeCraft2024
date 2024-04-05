#pragma once
#ifndef CODECRAFTSDK_CONFIG_H
#define CODECRAFTSDK_CONFIG_H

#include <random>
#include <chrono>
#include <future>
#include <ostream>

#ifdef DEBUG_
#define DEBUG if(true)
#define ASSERT(CON) if(!(CON)) { std::cerr << "ASSERTION FAILURE: " << #CON << std::endl;exit(-1);}
const size_t RANDOM_SEED = 42;
// const size_t RANDOM_SEED = std::random_device{}();
#else
#define DEBUG if(false)
#define ASSERT
const size_t RANDOM_SEED = std::random_device{}();
#endif

using u8 = u_int8_t;
using u16 = u_int16_t;
using u32 = u_int32_t;
using u64 = u_int64_t;
using f80 = long double;

using index_t = u16;
constexpr index_t no_index = -1;

constexpr int N = 200;
// constexpr int ROBOT_NUM = 10;
// constexpr int BERTH_NUM = 10;
// constexpr int SHIP_NUM = 5;
constexpr int MAX_FRAME = 15000;

constexpr int ROBOT_COST = 2000;
constexpr int SHIP_COST = 8000;

extern int stamp, money;
extern std::mt19937 eng;
extern int SHIP_CAPACITY;
extern std::chrono::high_resolution_clock::time_point start_time;
extern std::vector<std::future<void>> async_pool;

extern int tot_score;

#endif//CODECRAFTSDK_CONFIG_H
