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

struct Ship {
    std::array<Position, 6> area;
    using Area = std::array<Position, 6>;

    static Area getArea(Position center_t, Direction dir_t) {
        Area ans;
        auto front = dir_t;
        auto right = next(dir_t);
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

    static std::pair<int, bool> getId(Position center_t, Direction dir_t) {
        bool multi = false;
        if(center_t.outside()) { return {-1, false}; }
        for(auto pos : getArea(center_t, dir_t)) {
            if(pos.outside() || !SHIP_BLOCK.count(Atlas::atlas.maze[pos])) {
                return {-1, false};
            }
            multi |= (Atlas::atlas.maze[pos] == MAP_SYMBOLS::SEA_MULTI || Atlas::atlas.maze[pos] == MAP_SYMBOLS::SEA_GROUND_MULTI);
        }
        return {(int) center_t + Id(dir_t) * N * N, multi};
    }

    void updateArea() {

    }

    Ship() = default;

    friend auto &operator>>(std::istream &in, Ship &b) {
        int id, good, x, y, dir, sta;
        in >> id >> good >> x >> y >> dir >> sta;
        return in;
    }
};

struct Ships : public std::vector<Ship> {
    using vector::vector;
    static Ships ships;

     struct Edge {
         int from, to;
         int w;
     };

    DirectedGraph<Edge> graph;

    Ships() = default;

    auto init() {
        return std::async(std::launch::async, [this] {
            graph.resize(N * N * 4);
            for(int i = 0; i < N; i++) {
                for(int j = 0; j < N; j++) {
                    Position pos{i, j};
                    for(auto dir : Move) {
                        auto [id, at] = Ship::getId(pos, dir);
                        if(id < 0) { continue; }
                        // 前进
                        auto last = Ship::getId(pos + dir, dir).first;
                        if(last >= 0) { graph.add_edge({id, last, at ? 2 : 1}); }
                        // 右转
                        last = Ship::getId(pos + dir + dir, next(dir)).first;
                        if(last >= 0) { graph.add_edge({id, last, 1}); }
                        // 左传
                        last = Ship::getId(pos + dir + next(dir), prev(dir)).first;
                        if(last >= 0) { graph.add_edge({id, last, 1}); }
                        // dept传送
                        // berth传送
                    }
                }
            }
        });
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
 * version 2:
 * 1. 设定一个船在N*N*4的超平面上跑Dijkstra
 *
 * BUGS:
 *  1. [*已解决] Ship每次load最多loading_speed个货物, 但是要看berth够不够
 *  2. [*已修复] Ship在结束前返回的策略不够精确
 * */


#endif//CODECRAFTSDK_SHIP_HPP
