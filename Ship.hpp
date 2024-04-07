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

    using Area = std::array<Position, 6>;
    ShipMode mode = WAITING;
    int id;
    Area area;
    int load_num = 0;
    int load_value = 0;
    int status;
    int target = 0;
    Position pos;
    Direction dir;
    char output[20];

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

    static std::set<char> SHIP_BLOCK_SYM;
    static std::set<char> SHIP_MULTI_SYM;

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
        int dir_t = 0, load_num = 0;
        in >> b.id >> load_num >> b.pos >> dir_t >> b.status;
        b.dir = Move[dir_t];
        // std::cerr << "berth: " << b.target << std::endl;
        // std::cerr << "status: " << b.status << std::endl;
        // std::cerr << b.load_num << " " << load_num << std::endl;
        if(load_num != 0) ASSERT(b.load_num == load_num);
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
             MULTI_TURN_LEFT,
             TURN_RIGHT,
             MULTI_TURN_RIGHT,
             DEPT,
             BERTH,
             ACTION_NUM,
         };
         int to;
         TYPE type;

         void toString(Ship &ship) const {
             switch(type) {
             case Action::FORWARD:
             case Action::MULTI_FORWARD:
                 snprintf(ship.output, sizeof(Ship::output), "ship %d", ship.id);
                 break;
             case Action::TURN_LEFT:
             case Action::MULTI_TURN_LEFT:
                 snprintf(ship.output, sizeof(Ship::output), "rot %d 1", ship.id);
                 break;
             case Action::TURN_RIGHT:
             case Action::MULTI_TURN_RIGHT:
                 snprintf(ship.output, sizeof(Ship::output), "rot %d 0", ship.id);
                 break;
             case Action::DEPT:
                 snprintf(ship.output, sizeof(Ship::output), "dept %d", ship.id);
                 break;
             case Action::BERTH:
                 snprintf(ship.output, sizeof(Ship::output), "berth %d", ship.id);
                 break;
             default:;
             }
         }
     };
     std::array<int, Action::ACTION_NUM> action_cost = {1, 2, 1, 2, 1, 2, -1, -1};

    DirectedGraph<Edge> rev_graph;
    std::vector<std::vector<Action>> graph;
    std::vector<std::vector<int>> berth_dis;
    std::vector<std::vector<int>> commit_dis;

    float updateTarget(Ship & ship) {
        if(ship.target < berth_dis.size()) {
            Berths::berths[ship.target].occupied = nullptr;
        }
        int target = 0;
        auto calc = [&](int berth_id){
            auto &berth = Berths::berths[berth_id];
            auto berth_hold = berth.notified + berth.cargo.size();
            return (float) (berth.notified_value + berth.cargo_value) /
                   ((float) berth_dis[berth_id][Ship::getId(ship.pos, ship.dir).first] +
                            (float) berth_hold / (float) berth.loading_speed);
        };
        float val = 0.f;
        for(int i = 0; i < Berths::berths.size(); i++) {
            auto &berth = Berths::berths[i];
            if(berth.occupied) { continue; }
            float val_t = calc(i);
            if(val_t > val) {
                val = val_t;
                target = i;
            }
        }
        ship.target = target;
        Berths::berths[target].occupied = &ship;
        return val;
    }
    int selectCommit(Ship & ship) {
        if(ship.target < berth_dis.size()) {
            Berths::berths[ship.target].occupied = nullptr;
        }
        int target = 0;
        auto id = Ship::getId(ship.pos, ship.dir).first;
        for(int i = 1; i < commit_point.size(); i++) {
            // auto d = commit_dis[i][id];
            if(commit_dis[i][id] < commit_dis[target][id]) {
                target = i;
            }
        }
        target += berth_dis.size();
        ship.target = target;
        return commit_dis[target - berth_dis.size()][id];
    }

    void add_edge(int f, int t, Action::TYPE type) {
        if(f == -1 || t == -1) {
            return;
        }
        int cost = action_cost[type];
        if(cost == -1){
            auto[p1, _1] = Ship::unpack(f);
            auto[p2, _2] = Ship::unpack(t);
            cost = 2 * (std::abs(p1.first - p2.first) + std::abs(p1.second - p2.second));
        }
        rev_graph.add_edge({t, f, cost});
        graph[f].push_back({t, type});
    }

    Ships() {
        reserve(20);
    }

    auto init() {
        return std::async(std::launch::async, [this] {
            rev_graph.resize(N * N * 4);
            graph.resize(N * N * 4);
            for(int i = 0; i < N; i++) {
                for(int j = 0; j < N; j++) {
                    Position pos{i, j};
                    for(auto dir : Move) {
                        auto id = Ship::getId(pos, dir).first;
                        if(id < 0) { continue; }
                        // 前进
                        auto [last, at] = Ship::getId(pos + dir, dir);
                        add_edge(id, last, at ? Action::MULTI_FORWARD : Action::FORWARD);
                        // 右转
                        std::tie(last, at) = Ship::getId(pos + dir + dir, next(dir));
                        add_edge(id, last, at ? Action::MULTI_TURN_RIGHT : Action::TURN_RIGHT);
                        // 左传
                        std::tie(last, at) = Ship::getId(pos + dir + next(dir), prev(dir));
                        add_edge(id, last, at ? Action::MULTI_TURN_LEFT : Action::TURN_LEFT);
                        // dept传送
                        // berth传送
                        for(auto &berth : Berths::berths){
                            if(berth.around(pos)){
                                std::tie(last, at) = Ship::getId(berth.pos, berth.dir);
                                add_edge(id, last, Action::BERTH);
                            }
                        }
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

    void check_force_back(Ship &ship){
        int target = ship.target;
        if(target < berth_dis.size()){
            int dis = selectCommit(ship);
            if(dis < MAX_FRAME - stamp - 5){
                ship.target = target;
            }else{
                ship.mode = Ship::SAILING;
            }
        }
    }

    auto resolve() {
        return std::async(std::launch::async, [this] {
            for(auto & ship : *this) {
                ship.output[0] = '\0';
                if(ship.status == 1) {
                    continue;
                }
                check_force_back(ship);
                switch(ship.mode) {
                case Ship::SAILING: {
                    // if(ship.target < berth_dis.size()){
                    //     updateTarget(ship);
                    // }
                    auto [id, dir] = Ship::getId(ship.pos, ship.dir);
                    bool is_commit = ship.target >= berth_dis.size();
                    auto & dis = (is_commit ? commit_dis[ship.target - berth_dis.size()] : berth_dis[ship.target]);
                    if(dis[id] == 0) {
                        if(is_commit) {
                            DEBUG tot_score += ship.load_value;
                            ship.load_num = ship.load_value = 0;
                            updateTarget(ship);
                        }
                        else {
                            snprintf(ship.output, sizeof(Ship::output), "berth %d", ship.id);
                            ship.mode = Ship::LOADING;
                        }
                    }else if(dis[id] > 0 && !graph[id].empty()){
                        int next_idx = 0, next_dis = -1;
                        std::vector<int> sf(graph[id].size());
                        std::iota(sf.begin(), sf.end(), 0);
                        std::shuffle(sf.begin(), sf.end(), eng);
                        for(auto i : sf) {
                            auto i_dis = dis[graph[id][i].to];
                            if(next_dis == -1 || (i_dis != -1 && i_dis < next_dis)) {
                                next_dis = i_dis;
                                next_idx = i;
                            }
                        }
                        auto & edge = graph[id][next_idx];
                        edge.toString(ship);
                    }
                    break;
                }
                case Ship::LOADING: {
                    if(ship.load_num == SHIP_CAPACITY) {
                        selectCommit(ship);
                        ship.mode = Ship::SAILING;
                    }
                    else {
                        auto[cnt, value] = Berths::berths[ship.target].get_load(SHIP_CAPACITY - ship.load_num);
                        ship.load_num += cnt;
                        ship.load_value += value;
                        if(Berths::berths[ship.target].cargo.empty()) {
                            auto val1 = updateTarget(ship);
                            int target_t = ship.target;
                            auto val2 = selectCommit(ship);
                            if(val1 > (float) ship.load_value / (float) val2){
                                ship.target = target_t;
                            }
                            ship.mode = Ship::SAILING;
                        }
                    }
                    break;
                }
                default:
                    updateTarget(ship);
                    ship.mode = Ship::SAILING;
                }
            }
        });
    }

    void new_ship(Position p) {
        // Ship ship{
        //         .id = (int)size(),
        //         .pos = p,
        //         .dir = RIGHT
        // };
        Ship ship;
        ship.id = (int) size();
        ship.pos = p;
        ship.dir = RIGHT;
        updateTarget(ship);
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
 * 1. [*已实现] 设定一个船在N*N*4的超平面上跑Dijkstra
 * 2. 基于version1写一个决策分配模型
 * 3. 解决多船决策和避障
 *
 * BUGS:
 *  1. [*已解决] Ship每次load最多loading_speed个货物, 但是要看berth够不够
 *  2. [*已修复] Ship在结束前返回的策略不够精确
 * */


#endif//CODECRAFTSDK_SHIP_HPP
