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
    enum ShipMode {
        WAITING,
        SAILING,
        LOADING,
    };

    std::array<Position, 6> area;
    ShipMode mode = WAITING;
    int last_action_num = 0;
    int id;
    int load_num = 0;
    int status;
    int target = 0;
    using Area = std::array<Position, 6>;
    Position pos;
    Direction dir;
    std::string output = "";
    int wait_time = 0;

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

    static inline std::set<char> SHIP_BLOCK_SYM{
            MAP_SYMBOLS::SEA,
            MAP_SYMBOLS::SEA_MULTI,
            MAP_SYMBOLS::SHIP,
            MAP_SYMBOLS::SEA_GROUND,
            MAP_SYMBOLS::SEA_GROUND_MULTI,
            MAP_SYMBOLS::COMMIT,
            MAP_SYMBOLS::BERTH,
    };

    static inline std::set<char> SHIP_MULTI_SYM{
            MAP_SYMBOLS::SEA_MULTI,
            MAP_SYMBOLS::SHIP,
            MAP_SYMBOLS::SEA_GROUND_MULTI,
            MAP_SYMBOLS::BERTH,
    };

    static std::pair<int, bool> getId(Position center_t, Direction dir_t) {
        bool multi = false;
        if(center_t.outside()) { return {-1, false}; }
        for(auto pos : getArea(center_t, dir_t)) {
            if(pos.outside() || !SHIP_BLOCK_SYM.count(Atlas::atlas.maze[pos])) {
                return {-1, false};
            }
            if(!multi) {multi |= SHIP_MULTI_SYM.count(Atlas::atlas.maze[pos]); }
        }
        return {(int) center_t + Id(dir_t) * N * N, multi};
    }

    static std::pair<Position, Direction> unpack(int num) {
        return {num % (N * N), Move[num / (N * N)]};
    }

    Ship() = default;

    friend auto &operator>>(std::istream &in, Ship &b) {
        int dir_t = 0;
        in >> b.id >> b.load_num >> b.pos >> dir_t >> b.status;
        b.dir = Move[dir_t];
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

     struct Action {
         enum TYPE{
             FORWARD,
             MULTI_FORWARD,
             TURN_LEFT,
             TURN_RIGHT,
             DEPT,
             ACTION_NUM
         };
         int to;
         TYPE type;

         std::string toString(int id) const {
             switch(type) {
             case FORWARD:
             case MULTI_FORWARD:
                 return "ship " + std::to_string(id);
             case TURN_LEFT:
                 return "rot " + std::to_string(id) + " 1";
             case TURN_RIGHT:
                 return "rot " + std::to_string(id) + " 1";
             default:
                 return "";
             }
         }
     };
     std::array<int, Action::ACTION_NUM> action_cost = {1, 2, 1, 1, -1};

    DirectedGraph<Edge> rev_graph;
    std::vector<std::vector<Action>> graph;
    std::vector<std::vector<int>> berth_dis;
    std::vector<std::vector<int>> commit_dis;

    void add_edge(int f, int t, Action::TYPE type) {
        if(f == -1 || t == -1) {
            return;
        }
        rev_graph.add_edge({t, f, action_cost[type]});
        graph[f].push_back({t, type});
    }

    Ships() = default;

    auto init() {
        return std::async(std::launch::async, [this] {
            rev_graph.resize(N * N * 4);
            graph.resize(N * N * 4);
            for(int i = 0; i < N; i++) {
                for(int j = 0; j < N; j++) {
                    Position pos{i, j};
                    for(auto dir : Move) {
                        auto [id, at] = Ship::getId(pos, dir);
                        if(id < 0) { continue; }
                        // 前进
                        auto last = Ship::getId(pos + dir, dir).first;
                        add_edge(id, last, at ? Action::MULTI_FORWARD : Action::FORWARD);
                        // 右转
                        last = Ship::getId(pos + dir + dir, next(dir)).first;
                        add_edge(id, last, Action::TURN_RIGHT);
                        // 左传
                        last = Ship::getId(pos + dir + next(dir), prev(dir)).first;
                        add_edge(id, last, Action::TURN_LEFT);
                        // dept传送
                        // berth传送
                    }
                }
            }
            for(auto & berth : Berths::berths) {
                berth_dis.emplace_back(rev_graph.dijkstra(Ship::getId(berth.pos, berth.dir).first));
            }
            for(auto & commit : commit_point) {
                commit_dis.emplace_back(graph.size(), -1);
                auto & dis = commit_dis[commit_dis.size() - 1];
                for(auto dir : Move){
                    auto dir_dis = rev_graph.dijkstra(Ship::getId(commit, dir).first);
                    for(int i = 0; i < dis.size(); i++) {
                        if(dis[i] == -1 || (dir_dis[i] != -1 && dir_dis[i] < dis[i])) {
                            dis[i] = dir_dis[i];
                        }
                    }
                }
            }
        });
    }

    auto resolve() {
        return std::async(std::launch::async, [this] {
            for(auto & ship : *this) {
                switch(ship.mode) {
                case Ship::SAILING: {
                    if(ship.last_action_num > 0) {
                        ship.output = "";
                        ship.last_action_num--;
                        break;
                    }
                    auto [id, dir] = Ship::getId(ship.pos, ship.dir);
                    bool is_commit = ship.target >= berth_dis.size();
                    auto & dis = (is_commit ? commit_dis[ship.target - berth_dis.size()] : berth_dis[ship.target]);
                    if(dis[id] == 0) {
                        if(is_commit) {
                            ship.target = 0;
                        }
                        else {
                            ship.wait_time = 100;
                            ship.mode = Ship::LOADING;
                        }
                    }
                    else {
                        int next_idx = 0, next_dis = dis[graph[id][0].to];
                        for(int i = 1; i < graph[id].size(); i++) {
                            auto i_dis = dis[graph[id][i].to];
                            if(next_dis == -1 || (i_dis != -1 && i_dis < next_dis)) {
                                next_dis = i_dis;
                                next_idx = i;
                            }
                        }
                        auto & edge = graph[id][next_idx];
                        ship.output = edge.toString(ship.id);
                        ship.last_action_num = action_cost[edge.type] - 1;
                    }
                    break;
                }
                case Ship::LOADING: {
                    if(ship.wait_time == 0) {
                        ship.target++;
                        ship.mode = Ship::SAILING;
                    }
                    else {
                        ship.wait_time--;
                    }
                    break;
                }
                default:
                    ship.target = 0;
                    ship.mode = Ship::SAILING;
                }
            }
        });
    }

    void new_ship(Position p) {
        Ship ship{
                .id = (int)size(),
                .pos = p,
                .dir = RIGHT
        };
        push_back(ship);
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
