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
#include "Robot.hpp"

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
    int target = 1000;
    Position pos;
    Direction dir;

    Position last_dis_pos;
    Direction last_dis_dir;
    char output[20];

    std::vector<int> dis;
    int dis_update_stamp = -1;

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

    void updateArea() {
        area = getArea(pos, dir);
    }

    static std::set<char> SHIP_BLOCK_SYM;
    static std::set<char> SHIP_MULTI_SYM;

    static std::pair<int, bool> getId(Position center_t, Direction dir_t) {
        bool multi = false;
        if(center_t.outside()) { return {-1, false}; }
        for(auto pos: getArea(center_t, dir_t)) {
            if(pos.outside() || !SHIP_BLOCK_SYM.count(Atlas::atlas.maze[pos])) {
                return {-1, false};
            }
            if(!multi) { multi |= SHIP_MULTI_SYM.count(Atlas::atlas.maze[pos]); }
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
        enum TYPE {
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
        int form;
        int to;
        TYPE type;
        int w;

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
                DEBUG std::cerr << "dept " << stamp << '\n';
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
    DirectedGraph<Action> graph;
    std::vector<std::vector<int>> berth_dis;
    std::vector<std::vector<int>> commit_dis;
    // uint32_t tick;
    std::array<index_t, N * N> occupy_table;

    // std::vector<bool> multi_table;
    // bool multi(Position pos) {
    //     return multi_table[pos];
    // }

    std::vector<int> getAbstractPos(Position pos, Direction dir = {0, 0}) {
        if(dir != 0) {
            return {(int) pos + Id(dir) * N * N};
        } else {
            std::vector<int> ans;
            ans.reserve(Move.size());
            for(auto dir_t: Move) {
                ans.push_back((int) pos + Id(dir_t) * N * N);
            }
            return ans;
        }
    }

    void updateDistance(Ship &ship) {
        if(ship.dir == ship.last_dis_dir && ship.pos == ship.last_dis_pos) {
            return;
        }
        ship.last_dis_dir = ship.dir;
        ship.last_dis_pos = ship.pos;
        auto id = getAbstractPos(ship.pos, ship.dir)[0];
        ship.dis = graph.dijkstra_plus_({id}, get_occupy_fun(ship.id));
    }

    bool updateTarget(Ship &ship, int *time = nullptr) {
        if(ship.target < berth_dis.size()) {
            Berths::berths[ship.target].occupied = nullptr;
        }
        int target = 0;
        auto id = Ship::getId(ship.pos, ship.dir).first;
        auto &dis = ship.dis;
        if(ship.dis_update_stamp < stamp) {
            if(size() > 1) {
                updateDistance(ship);
            }
            ship.dis_update_stamp = stamp;
        }
        auto calc = [&](int berth_id) {
            auto &berth = Berths::berths[berth_id];
            auto berth_hold = berth.notified + (int) berth.cargo.size();
            auto load_value = berth.notified_value + berth.cargo_value;
            // for(int i = 0; i < berth.cargo.size(); i++) {
            //     load_value += (float) berth.cargo[i].value;
            // }
            auto d = size() > 1 ? dis[berth.abstract_pos] : berth_dis[berth.id][id];
            return (float) load_value / ((float) d + (float) std::min(berth_hold, SHIP_CAPACITY - ship.load_num) / (float) berth.loading_speed);
        };
        auto val = 0.f;
        bool finish = false;
        for(int i = 0; i < Berths::berths.size(); i++) {
            auto &berth = Berths::berths[i];
            if(size() > 1) {
                if(berth.occupied || dis[berth.abstract_pos] == -1) { continue; }
            } else {
                if(berth.occupied || berth_dis[i][id] == -1) { continue; }
            }
            finish = true;
            auto val_t = calc(i);
            if(val_t > val) {
                val = val_t;
                target = i;
                if(time) {
                    if(size() > 1) {
                        *time = dis[berth.abstract_pos];
                    } else {
                        *time = berth_dis[i][id];
                    }
                }
            }
        }
        ship.target = target;
        Berths::berths[target].occupied = &ship;
        return finish;
    }

    bool selectCommit(Ship &ship, int *time = nullptr) {
        if(ship.target < berth_dis.size()) {
            Berths::berths[ship.target].occupied = nullptr;
        }
        int target = 1000;
        auto id = Ship::getId(ship.pos, ship.dir).first;
        auto &dis = ship.dis;
        if(ship.dis_update_stamp < stamp) {
            if(size() > 1) {
                updateDistance(ship);
            }
            ship.dis_update_stamp = stamp;
        }
        for(int i = 0; i < commit_point.size(); i++) {
            auto commit_abstract_pos = getAbstractPos(commit_point[i]);
            bool valid = false;
            if(size() > 1) {
                for(auto p: commit_abstract_pos) {
                    if(dis[p] != -1) {
                        valid = true;
                    }
                }
            } else {
                valid = (commit_dis[i][id] != -1);
            }
            if(!valid) {
                continue;
            }
            if(target >= 1000 || commit_dis[i][id] < commit_dis[target][id]) {
                target = i;
                if(time) { *time = commit_dis[i][id]; }
            }
        }
        if(target >= 1000) {
            return false;
        }
        target += (int) berth_dis.size();
        ship.target = target;
        return true;
    }

    void add_edge(int f, int t, Action::TYPE type) {
        if(f == -1 || t == -1) {
            return;
        }
        int cost = action_cost[type];
        if(cost == -1) {
            auto [p1, _1] = Ship::unpack(f);
            auto [p2, _2] = Ship::unpack(t);
            cost = 2 * (std::abs(p1.first - p2.first) + std::abs(p1.second - p2.second));
        }
        rev_graph.add_edge({t, f, cost});
        graph[f].push_back({f, t, type, cost});
    }

    void updateOccupyTable(bool fill) {
        for(auto &ship: *this) {
            for(auto &p: Ship::getArea(ship.pos, ship.dir)) {
                if(Ship::SHIP_MULTI_SYM.count(Atlas::atlas.maze[p])) {
                    continue;
                } else {
                    if(fill) {
                        occupy_table[p] = ship.id;
                    } else {
                        occupy_table[p] = no_index;
                    }
                }
            }
        }
    }

    Ships() {
        reserve(20);
    }

    auto init() {
        return std::async(std::launch::async, [this] {
            occupy_table.fill(no_index);
            // multi_table.resize(N * N);
            rev_graph.resize(N * N * 4);
            graph.resize(N * N * 4);
            Bitset<N * N> vis{};
            for(int i = 0; i < N; i++) {
                for(int j = 0; j < N; j++) {
                    Position pos{i, j};
                    // multi_table[pos] = Ship::SHIP_MULTI_SYM.count(Atlas::atlas.maze[pos]);
                    for(auto dir: Move) {
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
                        // {
                        //     Queue<Position, 3 * N> q;
                        //     vis.reset(0, N * N);
                        //     q.push(pos);
                        //     while(!q.empty()) {
                        //         auto u = q.pop();
                        //         if(Ship::SHIP_MULTI_SYM.count(Atlas::atlas.maze[u])) {
                        //             auto ab_last = getAbstractPos(u);
                        //             for(auto k: ab_last) {
                        //                 auto [p, d] = Ship::unpack(k);
                        //                 bool fl = true;
                        //                 for(auto x: Ship::getArea(p, d)) {
                        //                     if(!Ship::SHIP_MULTI_SYM.count(x)) {
                        //                         fl = false;
                        //                         break;
                        //                     }
                        //                 }
                        //                 if(!fl) { continue; }
                        //                 add_edge(id, k, Action::DEPT);
                        //                 break;
                        //             }
                        //             break;
                        //         }
                        //         vis.set(u);
                        //         for(auto &move: Move) {
                        //             auto v = u + move;
                        //             if(v.outside() || vis.test(v)) { continue; }
                        //             q.push(v);
                        //         }
                        //     }
                        // }
                        // berth传送
                        for(auto &berth: Berths::berths) {
                            if(berth.around(pos)) {
                                std::tie(last, at) = Ship::getId(berth.pos, berth.dir);
                                add_edge(id, last, Action::BERTH);
                            }
                        }
                    }
                }
            }
            for(auto &berth: Berths::berths) {
                berth.abstract_pos = Ship::getId(berth.pos, berth.dir).first;
                berth_dis.emplace_back(rev_graph.dijkstra(berth.abstract_pos));
            }
            for(auto &commit: commit_point) {
                commit_dis.emplace_back(graph.size(), -1);
                auto &dis = commit_dis[commit_dis.size() - 1];
                for(auto dir: Move) {
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

    void check_force_back(Ship &ship) {
        int target = ship.target, time;
        if(target < berth_dis.size() && selectCommit(ship, &time)) {
            if(time < MAX_FRAME - stamp - (size() > 1 ? 50 : 5)) {
                ship.target = target;
            } else {
                ship.mode = Ship::SAILING;
            }
        }
    }

    void setDept(Ship &ship) {
        sprintf(ship.output, "dept %d", ship.id);
    }

    std::function<bool(int)> get_occupy_fun(int idx) {
        return [this, idx](int pos_id) -> bool {
            auto [pos, dir] = Ship::unpack(pos_id);
            for(auto &p: Ship::getArea(pos, dir)) {
                if(this->occupy_table[p] != no_index && this->occupy_table[p] != idx) {
                    return true;
                }
            }
            return false;
        };
    }

    Action getNextAction(Ship &ship) {
        updateDistance(ship);
        std::vector<int> target_abstract_pos = {};
        if(ship.target >= berth_dis.size()) {
            target_abstract_pos = getAbstractPos(commit_point[ship.target - berth_dis.size()]);
        } else {
            target_abstract_pos = getAbstractPos(Berths::berths[ship.target].pos, Berths::berths[ship.target].dir);
        }
        int target = target_abstract_pos[0];
        auto &dis = ship.dis;
        for(auto p: target_abstract_pos) {
            if(dis[target] == -1 || (dis[p] != -1 && dis[p] < dis[target])) {
                target = p;
            }
        }
        auto id = getAbstractPos(ship.pos, ship.dir).front();
        int last = target;
        while(target != id) {
            // std::vector<int> valid_point;
            // for(auto & edge : rev_graph[target]) {
            //     if(dis[edge.to] != -1 && dis[edge.to] <= dis[target]) {
            //         valid_point.push_back(edge.to);
            //     }
            // }
            // std::shuffle(valid_point.begin(), valid_point.end(), eng);
            auto next = rev_graph[target][0];
            auto next_val = dis[rev_graph[target][0].to];
            for(int i = 1; i < rev_graph[target].size(); i++) {
                auto val = dis[rev_graph[target][i].to];
                if(next_val == -1 || (val != -1 && val < next_val)) {
                    next_val = val;
                    next = rev_graph[target][i];
                } else if(val != -1 && val <= next_val) {
                    std::uniform_int_distribution<int> u(0, 1);
                    if(u(eng)) {
                        next_val = val;
                        next = rev_graph[target][i];
                    }
                }
            }
            last = target;
            target = next.to;
        }
        for(auto &act: graph[id]) {
            if(act.to == last) {
                return act;
            }
        }
        return {};
    }

    void allShipDis() {
        std::vector<std::future<void>> ship_f;
        for(auto &ship: *this) {
            if(ship.status == 1) {
                continue;
            }
            ship_f.emplace_back(std::async(std::launch::async, [this, &ship] {
                updateDistance(ship);
            }));
        }
        for(auto &f: ship_f) {
            f.wait();
        }
    }
    // template<int idx>
    // bool occupy_fun(int pos_id) {
    //
    // }

    auto resolve() {
        return std::async(std::launch::async, [this] {
            // auto start_time = std::chrono::system_clock::now();
            updateOccupyTable(true);
            for(auto &ship: *this) {
                if(ship.status == 1) {
                    continue;
                }
                ship.updateArea();
            }
            allShipDis();
            for(auto &ship: *this) {
                ship.output[0] = '\0';
                if(ship.status == 1) {
                    continue;
                }
                check_force_back(ship);
                switch(ship.mode) {
                case Ship::SAILING: {
                    auto [id, dir] = Ship::getId(ship.pos, ship.dir);
                    bool is_commit = ship.target >= berth_dis.size();
                    std::vector<int> dis;
                    int real_dis = -1;
                    if(size() > 1) {
                        std::vector<int> target_abstract_pos = {};
                        if(is_commit) {
                            target_abstract_pos = getAbstractPos(commit_point[ship.target - berth_dis.size()]);
                        } else {
                            if(Berths::berths[ship.target].occupied != &ship) {
                                updateTarget(ship);
                            }
                            target_abstract_pos = getAbstractPos(Berths::berths[ship.target].pos, Berths::berths[ship.target].dir);
                        }
                        updateDistance(ship);
                        dis = ship.dis;
                        for(auto p: target_abstract_pos) {
                            if(real_dis == -1 || (dis[p] != -1 && dis[p] < real_dis)) {
                                real_dis = dis[p];
                            }
                        }
                    } else {
                        dis = is_commit ? commit_dis[ship.target - berth_dis.size()] : berth_dis[ship.target];
                        real_dis = dis[id];
                    }
                    if(real_dis == -1) {
                        bool success = false;
                        if(is_commit) {
                            success = selectCommit(ship);
                        } else {
                            success = updateTarget(ship);
                        }
                        if(!success) {
                            setDept(ship);
                        }
                    } else if(real_dis == 0) {
                        if(is_commit) {
                            DEBUG tot_score += ship.load_value;
                            ship.load_num = ship.load_value = 0;
                            updateTarget(ship);
                        } else {
                            snprintf(ship.output, sizeof(Ship::output), "berth %d", ship.id);
                            ship.mode = Ship::LOADING;
                        }
                    } else if(real_dis > 0 && !graph[id].empty()) {
                        Action edge;
                        if(size() > 1) {
                            edge = getNextAction(ship);
                        } else {
                            int next_idx = 0, next_dis = -1;
                            std::vector<int> sf(graph[id].size());
                            std::iota(sf.begin(), sf.end(), 0);
                            std::shuffle(sf.begin(), sf.end(), eng);
                            for(auto i: sf) {
                                auto i_dis = dis[graph[id][i].to];
                                if(next_dis == -1 || (i_dis != -1 && i_dis < next_dis)) {
                                    next_dis = i_dis;
                                    next_idx = i;
                                }
                            }
                            edge = graph[id][next_idx];
                        }
                        edge.toString(ship);
                        // std::tie(ship.pos, ship.dir) = Ship::unpack(edge.to);
                    }
                    break;
                }
                case Ship::LOADING: {
                    if(ship.load_num == SHIP_CAPACITY) {
                        selectCommit(ship);
                        ship.mode = Ship::SAILING;
                    } else {
                        auto [cnt, value] = Berths::berths[ship.target].get_load(SHIP_CAPACITY - ship.load_num);
                        ship.load_num += cnt;
                        ship.load_value += value;
                        if(Berths::berths[ship.target].cargo.empty()) {
                            if(Robots::robots.size() < MAX_ROBOT && ship.load_value + money >= ROBOT_COST) {
                                selectCommit(ship);
                            } else {
                                updateTarget(ship);
                            }
                            // auto val1 = updateTarget(ship);
                            // int target_t = ship.target;
                            // auto val2 = selectCommit(ship);
                            // if(val1 > (float) ship.load_value / (float) val2) {
                            //     ship.target = target_t;
                            // }
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
            updateOccupyTable(false);
            // for(int i = 0; i < size(); i++) {
            //     std::cerr << "ship" << i << " : " << (*this)[i].output << std::endl;
            // }
            // std::cerr << "use time: " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start_time).count() << std::endl;
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
