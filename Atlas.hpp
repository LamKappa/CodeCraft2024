#pragma once
#ifndef CODECRAFTSDK_ATLAS_HPP
#define CODECRAFTSDK_ATLAS_HPP

#include <array>
#include <bitset>
#include <cstring>
#include <future>
#include <iostream>
#include <istream>
#include <queue>
#include <set>
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
//    std::bitset<bitmap_size> bitmap;
    u16 dist[dist_size];

    std::future<void> f_lock;

    auto& distance(Flatten_Position A, Flatten_Position B) {
        if(A < B) std::swap(A, B);
        return dist[(1 + A) * A / 2 + B];
    }

    auto path(Position A, Position B) {
        for(auto& move: Move) {
            auto C = A + move;
            if(C.outside() || bitmap.test(C)) { continue; }
            if(distance(C, B) < distance(A, B)) {
                return move;
            }
        }
        return Position::npos;
    }

    auto init() {
        f_lock = std::async(std::launch::async, [this] {
            std::memset(dist, 0xff, dist_size * sizeof(u16));
        });
        bitmap.reset(0, bitmap_size);
    }

    auto build() {
        f_lock.wait();

        auto range_bfs = [this](u16 l, u16 r) {
            Queue<Flatten_Position, 3 * N> q;
            auto vised = bitmap;
            vised.set(0, l);
            for(int i = l; i < r; i++) {
                if(bitmap.test(i)) { continue; }
                vised.reset(i + 1, bitmap_size);
                vised.set(i);
                distance(i, i) = 0;
                q.push(i);
                while(!q.empty()) {
                    auto u = q.pop();
                    for(auto& move: Move) {
                        auto v = Position{u} + move;
                        if(v.outside() || bitmap.test(v) || vised.test(v)) { continue; }
                        vised.set(v);
                        distance(i, v) = distance(i, u) + 1;
                        q.push(v);
                    }
                }
            }
        };

        range_bfs(0, bitmap_size);
        // parallel by 2-cores
//        auto ft1 = std::async(std::launch::async, [&range_bfs] {
//            range_bfs(0, bitmap_size / 2);
//        });
//        auto ft2 = std::async(std::launch::async, [&range_bfs] {
//            range_bfs(bitmap_size / 2, bitmap_size);
//        });
//
//        ft1.wait();
//        ft2.wait();
    }
};

/**
 * idea:
 * 1. 采用扁平化索引, 省空间 (N^4会爆空间)
 * 2. BFS n^2 全局最短路
 *  优化
 *      1. 双线程, 一半空间, BFS只做向后搜索 (初始化16s)
 *      2. 自定义Bitset存储bitmap和vised, 支持/w常数的区间赋值 (初始化4.5s)
 *      3. 采用自定义的循环队列优化BFS的std::queue, 优化掉deque内存分配 (初始化4s)
 *      4. 将dist的初始化移至读入map之前多线程执行 (初始化3.5s)
 * 3. 采用四向枚举来确定最短路路径
 */

#endif//CODECRAFTSDK_ATLAS_HPP
