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

    std::bitset<N * N> bitmap;
    std::array<u16, N * N * N * N / 2> dist;

    auto bitmap_at(int i, int j) {
        return bitmap[Position{i, j}];
    }

    auto& distance(Position A, Position B) {
        if(A > B) swap(A, B);
        return dist[A * A / 2 + B];
    }

    auto build() {
        dist.fill(INF_DIS);

        std::future<void> ft1 = async(std::launch::async, [&] {
            std::queue<Position> q;
            decltype(bitmap) mask(0);
            for(int i = 0; i < N * N / 2; i++) {
                if(bitmap[i]) { continue; }
                auto vised = (mask[i] = true, mask);
                distance(i, i) = 0;
                q.emplace(i);
                while(!q.empty()) {
                    auto u = q.front();
                    q.pop();
                    for(auto& move: Move) {
                        Position v = u + move;
                        if(v.outside() || bitmap.test(v) || vised.test(v)) { continue; }
                        vised.set(v);
                        distance(i, v) = distance(i, u) + 1;
                        q.emplace(v);
                    }
                }
            }
        });

        std::future<void> ft2 = async(std::launch::async, [&] {
            std::queue<Position> q;
            decltype(bitmap) mask(0);
            mask[0] = 1;
            for(int i = 1; i < N * N / 2; i <<= 1) {
                mask |= mask << i;
            }
            mask |= mask << (N * N / 2 - (1 << 14));
            for(int i = N * N / 2; i < N * N; i++) {
                if(bitmap[i]) { continue; }
                auto vised = (mask[i] = true, mask);
                distance(i, i) = 0;
                q.emplace(i);
                while(!q.empty()) {
                    auto u = q.front();
                    q.pop();
                    for(auto& move: Move) {
                        Position v = u + move;
                        if(v.outside() || bitmap.test(v) || vised.test(v)) { continue; }
                        vised.set(v);
                        distance(i, v) = distance(i, u) + 1;
                        q.emplace(v);
                    }
                }
            }
        });

        ft1.wait();
        ft2.wait();
    }
};

#endif//CODECRAFTSDK_ATLAS_HPP
