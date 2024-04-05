#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>
#include <set>

#include "Berth.hpp"
#include "Config.h"
#include "Dijkstra.hpp"
// Hello World !!! 

struct Ship {
    static decltype(Atlas::atlas.maze) &maze;
    std::array<Position, 6> area;
    using Area = std::array<Position, 6>;

    static Area getArea(Position center_t, DIRECTION dir_t) {
        Area ans;
        auto front = Move[dir_t];
        auto right = Move[++dir_t];
        for(int i = 0; i < 3; i++) {
            ans[i * 2] = center_t;
            ans[i * 2 + 1] = center_t + right;
            center_t += front;
        }
        return ans;
    }

    static inline std::set<char> SHIP_BLOCK = {
            MAP_SYMBOLS::SEA,
            MAP_SYMBOLS::SEA_MULTI,
            MAP_SYMBOLS::SHIP,
            MAP_SYMBOLS::SEA_GROUND,
            MAP_SYMBOLS::SEA_GROUND_MULTI,
            MAP_SYMBOLS::COMMIT,
    };

    static std::pair<int, bool> getId(Position center_t, DIRECTION dir_t) {
        auto ls = getArea(center_t, dir_t);
        bool ans = false;
        for(auto pos : ls) {
            ans |= (maze[pos] == MAP_SYMBOLS::SEA_MULTI || maze[pos] == MAP_SYMBOLS::SEA_GROUND_MULTI);
            if(!SHIP_BLOCK.count(maze[pos])) {
                return {-1, false};
            }
        }
        return {(int) center_t + dir_t * N * N, ans};
    }

    void updateArea() {

    }

    Ship() = default;

    friend auto &operator>>(std::istream &in, Ship &b) {
        return in;
    }
};
static decltype(Atlas::atlas.maze) & maze = Atlas::atlas.maze;

struct Ships : public std::vector<Ship> {
    using vector::vector;
    static Ships ships;

     struct Edge {
         int from, to;
         int w;
     };

    DirectedGraph<Edge> graph;

    Ships() = default;

    void init() {
         graph.resize(N * N * 4);
         for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                for(auto dir : {UP, DOWN, LEFT, RIGHT}) {
                    auto [id, at] = Ship::getId({i, j}, dir);
                    auto dir_t = dir;
                    //前进
                    auto last = Ship::getId(Position{i, j} + Move[dir], dir).first;
                    graph.add_edge({id, last, at ? 2 : 1});
                    //右转
                    ++dir_t;
                    last = Ship::getId(Position{i, j} + Move[dir] + Move[dir], dir_t).first;
                    graph.add_edge({id, last, 1});
                    //左传
                    dir_t = dir;
                    --dir_t;
                    last = Ship::getId(Position{i, j} + Move[dir] + Move[dir], dir_t).first;
                    graph.add_edge({id, last, 1});
                }
            }
         }
    }

    auto resolve() {
        return std::async(std::launch::async, [this] {
        });
    }
};

/**
 * idea:
 * 1. 设定mission, 当泊位回调wanted时分配
 *  状态机模型: SAILING -> WAITING / LOADING / QUEUEING; LOADING -> SAILING; QUEUEING -> LOADING
 *  Exception:
 *      1. 没有多船只等候分配的优化
 *      2. 没有船只提前分配的优化
 *      3. 目前状态机转移比较混乱, 主要依靠每帧的输入判断 (潜在的BUG)
 * 2. [*已实现] transport, 用于计算从s到t的最优寻路, 此情景下最多中转一次
 * 3. [*已实现] 调度机制, 货物运到的速度<<装载, 所以动态优先调度船去转载货物 (考虑港口间移动)
 * 4. [*已实现] 调度添加边界 (当要结束时, 只考虑目标点位能转载的量)
 *
 * BUGS:
 *  1. [*已解决] Ship每次load最多loading_speed个货物, 但是要看berth够不够
 *  2. [*已修复] Ship在结束前返回的策略不够精确
 * */


#endif//CODECRAFTSDK_SHIP_HPP
