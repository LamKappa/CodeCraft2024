#pragma once
#ifndef CODECRAFTSDK_ATLAS_HPP
#define CODECRAFTSDK_ATLAS_HPP

#include <algorithm>
#include <array>
#include <bitset>
#include <cstring>
#include <future>
#include <iostream>
#include <istream>
#include <queue>
#include <set>
#include <thread>
#include <tuple>

#include "Bitset.hpp"
#include "Config.h"
#include "Position.hpp"
#include "Queue.hpp"

enum MAP_SYMBOLS {
    GROUND = ' ',
    SEA = '*',
    WALL = '#',
    ROBOT = 'A',
    BERTH = 'B'
};

const std::set<char> BARRIER_SYM{MAP_SYMBOLS::SEA, MAP_SYMBOLS::WALL};

struct Atlas {
    static Atlas atlas;
    static constexpr u16 INF_DIS = -1;

    // C-Array optimize
    static constexpr int bitmap_size = N * N;
    static constexpr int dist_size = N * N * (N * N + 3) / 2;
    Bitset<bitmap_size> bitmap;
    u16 dist[dist_size];

    std::future<void> f_lock;
    std::array<u8, Move.size()> sf = {0, 1, 2, 3};

    inline auto& distance(Flatten_Position A, Flatten_Position B) {
        if(A < B) return dist[(1 + B) * B / 2 + A];
        return dist[(1 + A) * A / 2 + B];
    }

    auto path(Position A, Position B) {
        std::shuffle(sf.begin(), sf.end(), eng);
        for(auto sf_i: sf) {
            auto C = A + Move[sf_i];
            if(C.outside() || bitmap.test(C)) { continue; }
            if(distance(C, B) < distance(A, B)) {
                return Move[sf_i];
            }
        }
        return Position::npos;
    }

    auto around(Position p) {
        std::shuffle(sf.begin(), sf.end(), eng);
        std::vector<Position> around_move;
        around_move.emplace_back(Position::npos);
        for(auto sf_i: sf) {
            auto C = p + Move[sf_i];
            if(C.outside() || bitmap.test(C)) { continue; }
            around_move.emplace_back(Move[sf_i]);
        }
        return around_move;
    }

    auto init() {
        f_lock = std::async(std::launch::async, [this] {
            std::memset(dist, 0xff, dist_size * sizeof(u16));
        });
    }

    auto build() {
        f_lock.wait();

        auto range_bfs = [this](u16 l, u16 r) {
            Queue<Position, 3 * N> q;
            for(auto i = l; i < r; i++) {
                if(bitmap.test(i)) { continue; }
                auto vised = bitmap;
                vised.set(i);
                distance(i, i) = 0;
                q.push(i);
                while(!q.empty()) {
                    auto u = q.pop();
                    for(auto& move: Move) {
                        auto v = u + move;
                        if(v.outside() || vised.test(v)) { continue; }
                        distance(i, v) = distance(i, u) + 1;
                        vised.set(v);
                        q.push(v);
                    }
                }
            }
        };

        // parallel by 2-cores
        std::thread([&range_bfs] {
            range_bfs(0, bitmap_size * 0.2);
        }).detach();
        std::thread([&range_bfs] {
            range_bfs(bitmap_size * 0.2, bitmap_size * 0.4);
        }).detach();
        std::thread([&range_bfs] {
            range_bfs(bitmap_size * 0.4, bitmap_size * 0.6);
        }).detach();
        std::thread([&range_bfs] {
            range_bfs(bitmap_size * 0.6, bitmap_size * 0.8);
        }).detach();
        std::thread([&range_bfs] {
            range_bfs(bitmap_size * 0.8, bitmap_size);
        }).detach();

        std::this_thread::sleep_for(std::chrono::milliseconds(4500));
        // range_bfs(0, bitmap_size);
    }
};

/**
 * idea:
 * 1. 采用扁平化索引, 省空间 (N^4会爆空间)
 * 2. BFS n^2 全局最短路
 *  优化
 *      1. 双线程, 一半空间, BFS只做向后搜索 (初始化16s)
 *      2. 自定义Bitset存储bitmap和vised, 支持/w常数的区间赋值 (初始化4.5s)
 *      3. 采用自定义的循环队列优化BFS的std::queue, 优化掉deque内存分配 (初始化4.0s)
 *      4. 将dist的初始化移至读入map之前多线程执行, distance不用swap (初始化3.5s)
 * 3. 采用四向枚举来确定最短路路径
 *  优化
 *      1. path采用随机枚举顺序(避让优化)
 */

#endif//CODECRAFTSDK_ATLAS_HPP
