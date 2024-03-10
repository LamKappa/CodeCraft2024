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
#include <thread>
#include <tuple>

#include "Bitset.hpp"
#include "Config.h"
#include "Position.hpp"

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

    using Flatten_Position = u16;

    // C-Array optimize
    static constexpr int bitmap_size = N * N;
    static constexpr int dist_size = N * N * N * N / 2;
    Bitset<bitmap_size> bitmap;
    u16 dist[dist_size];

    std::future<void> f_lock;

    auto& distance(Flatten_Position A, Flatten_Position B) {
        if(A > B) return dist[B * B / 2 + A];
        return dist[A * A / 2 + B];
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

    auto init(){
        f_lock = std::async(std::launch::async, [this] {
            std::memset(dist, 0xff, dist_size * sizeof(u16));
        });
    }

    auto build() {
        f_lock.wait();

        auto range_bfs = [this](u16 l, u16 r) {
            std::queue<Position, std::deque<Position, std::allocator<Position>>> q;
            auto vised = bitmap;
            vised.set(0, l);
            for(int i = l; i < r; i++) {
                if(bitmap.test(i)) { continue; }
                vised.reset(i + 1, bitmap_size);
                vised.set(i);
                distance(i, i) = 0;
                q.emplace(i);
                while(!q.empty()) {
                    auto u = q.front();
                    q.pop();
                    for(auto& move: Move) {
                        auto v = u + move;
                        if(v.outside() || bitmap.test(v) || vised.test(v)) { continue; }
                        vised.set(v);
                        distance(i, v) = distance(i, u) + 1;
                        q.push(v);
                    }
                }
            }
        };

        // parallel by 2-cores
        auto ft1 = std::async(std::launch::async, [&range_bfs] {
            range_bfs(0, bitmap_size / 2);
        });
        auto ft2 = std::async(std::launch::async, [&range_bfs] {
            range_bfs(bitmap_size/ 2, bitmap_size);
        });

        ft1.wait();
        ft2.wait();
    }
};

#endif//CODECRAFTSDK_ATLAS_HPP
