#pragma once
#ifndef CODECRAFTSDK_ATLAS_HPP
#define CODECRAFTSDK_ATLAS_HPP

#include <array>
#include <bitset>
#include <future>
#include <istream>
#include <queue>
#include <set>
#include <thread>
#include <tuple>

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

    // C-Array optimize
    static constexpr int bitmap_size = N * N;
    static constexpr int dist_size = N * N * N * N / 2;
    bool bitmap[bitmap_size];
    u16 dist[dist_size];

    auto& bitmap_at(int i, int j) {
        return bitmap[Position{i, j}];
    }

    auto& distance(Position A, Position B) {
        if(A > B) swap(A, B);
        return dist[A * A / 2 + B];
    }

    auto build() {
        std::fill(dist, dist + dist_size, INF_DIS);

        auto range_bfs = [this](int l, int r) {
            std::queue<Position> q;
            decltype(bitmap) mask;
            std::fill(mask, mask + l, false);
            std::fill(mask + l, mask + bitmap_size, false);
            for(int i = l; i < r; i++) {
                if(bitmap[i]) { continue; }
                auto vised = (mask[i] = true, mask);
                distance(i, i) = 0;
                q.emplace(i);
                while(!q.empty()) {
                    auto u = q.front();
                    q.pop();
                    for(auto& move: Move) {
                        Position v = u + move;
                        if(v.outside() || bitmap[v] || vised[v]) { continue; }
                        vised[v] = true;
                        distance(i, v) = distance(i, u) + 1;
                        q.emplace(v);
                    }
                }
            }
        };

        std::future<void> ft1 = async(std::launch::async, [&range_bfs]{
            range_bfs(0, bitmap_size / 2);
        });
        std::future<void> ft2 = async(std::launch::async, [&range_bfs]{
            range_bfs(bitmap_size / 2, bitmap_size);
        });

        ft1.wait();
        ft2.wait();
    }
};

#endif//CODECRAFTSDK_ATLAS_HPP
