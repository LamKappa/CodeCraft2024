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
    GROUND              = '.',
    GROUND_MULTI        = '>',
    SEA                 = '*',
    SEA_MULTI           = '~',
    WALL                = '#',
    ROBOT               = 'R',
    SHIP                = 'S',
    BERTH               = 'B',
    BERTH_AROUND        = 'K',
    SEA_GROUND          = 'C',
    SEA_GROUND_MULTI    = 'c',
    COMMIT              = 'T',
};

const std::set<char> BARRIER_SYM{
        MAP_SYMBOLS::SEA,
        MAP_SYMBOLS::SEA_MULTI,
        MAP_SYMBOLS::SHIP,
        MAP_SYMBOLS::WALL,
        MAP_SYMBOLS::BERTH_AROUND,
        MAP_SYMBOLS::COMMIT,
};

extern std::vector<Position> robot_shop, ship_shop, commit_point;
extern int now_frame;

struct Atlas {
    static Atlas atlas;
    static constexpr u16 INF_DIS = -1;

    // C-Array optimize
    static constexpr int bitmap_size = N * N;
    static constexpr int dist_size = N * N * (N * N + 3) / 2;
    Bitset<bitmap_size> bitmap;
    char maze[bitmap_size];
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
        constexpr int PN = 7;
        for(auto i = 0; i < PN; i++) {
            async_pool.emplace_back(std::move(
                    std::async(std::launch::async, [&range_bfs, i] {
                        range_bfs(bitmap_size * i / PN, bitmap_size * (i + 1) / PN);
                    })));
        }
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
