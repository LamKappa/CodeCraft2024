#pragma once
#ifndef CODECRAFTSDK_ROBOT_HPP
#define CODECRAFTSDK_ROBOT_HPP

#include <array>
#include <future>
#include <iostream>

#include "Atlas.hpp"
#include "Berth.hpp"
#include "Config.h"
#include "Item.hpp"
#include "Position.hpp"

#define Robot_idea_1

struct Robot {
    Robot() = default;
    struct Mission {
        Mission() = default;
        enum MISSION_STATE {
            WAITING,
            IDLING,
            SEARCHING,
            CARRYING,
        };
        static Mission idle;

        MISSION_STATE mission_state{MISSION_STATE::IDLING};
        Robot *executor{nullptr};
        float reserved_value{0.f};
        std::deque<std::pair<long, index_t>> targets;
        Position next_move{Position::npos};

        [[nodiscard]] inline auto vacant() const {
            return mission_state == WAITING || mission_state == IDLING;
        }
#ifdef Robot_idea_1
        Mission create(decltype(executor) exec) {
            auto &mission = *this;
            if(mission.mission_state == CARRYING) { return exec->mission; }
            // if(mission.mission_state == SEARCHING) { return exec->mission; }
            for(auto [id, _]: mission.targets) {
                Items::items.find_by_id(id).occupied = nullptr;
            }
            mission = {SEARCHING, exec};
            auto calc_value = [](const Robot &robot, const Item &item, const Berth &berth) {
                Atlas &atlas = Atlas::atlas;
                float rate = 1.f - (float) (item.live_time() - atlas.distance(robot.pos, item.pos)) / Item::OVERDUE;
                rate = (float) std::pow(rate, 0.42);
                return (rate * (float) item.value) /
                       ((float) atlas.distance(robot.pos, item.pos) +
                        (float) atlas.distance(item.pos, berth.pos));
            };
            std::priority_queue<std::pair<decltype(reserved_value), decltype(targets)::value_type>> q;
            for(auto &item: Items::items) {
                if(item.deleted || item.live_time() < Atlas::atlas.distance(exec->pos, item.pos)) { continue; }
                float best_value = 0.f;
                index_t best_berth_id;
                for(auto &berth: Berths::berths) {
                    if(berth.disabled_pulling || Atlas::atlas.distance(exec->pos, berth.pos) == Atlas::INF_DIS) { continue; }
                    auto value = calc_value(*exec, item, berth);
                    if(value > best_value) {
                        best_value = value;
                        best_berth_id = berth.id;
                    }
                }
                if(best_value <= 0.f) { continue; }
                q.emplace(best_value, std::make_pair(item.unique_id, best_berth_id));
            }
            while(!q.empty()) {
                auto [value, target] = q.top();
                q.pop();
                auto &item = Items::items.find_by_id(target.first);
                auto robot_ptr = static_cast<Robot *>(item.occupied);
                if(robot_ptr == nullptr) {
                    mission.reserved_value = value;
                    mission.targets.assign(1, target);
                    item.occupied = exec;
                    break;
                } else {
                    if(robot_ptr->mission.mission_state == CARRYING) { continue; }
                    auto &berth = Berths::berths[target.second];
                    if(calc_value(*robot_ptr, item, berth) < value) {
                        mission.reserved_value = value;
                        mission.targets.assign(1, target);
                        item.occupied = exec;
                        robot_ptr->mission = idle;
                        robot_ptr->mission.create(robot_ptr);
                        break;
                    }
                }
            }
            if(mission.targets.empty()) { return mission = idle; }
            return mission;
        }
#endif
#ifdef Robot_idea_3
        Mission create(decltype(executor) exec) {
            auto &mission = *this;
            if(!mission.vacant()) { return mission; }
            // if(mission.mission_state == CARRYING) { return mission; }
            mission = {SEARCHING, exec, 0.f};
            for(auto [id, _]: exec->mission.targets) {
                Items::items.find_by_id(id).occupied = nullptr;
            }
            auto distance = [](auto p1, auto p2) -> int { return Atlas::atlas.distance(p1, p2); };
            for(auto &berth: Berths::berths) {
                if(berth.disabled_pulling || distance(exec->pos, berth.pos) == Atlas::INF_DIS) { continue; }
                std::vector dp(3 * Item::OVERDUE, 0.f);
                std::unordered_map<int, std::vector<long>> item_list;
                for(auto &item: Items::items) {
                    if(item.occupied) { continue; }
                    auto live_time = item.live_time();
                    auto berth_to_item = distance(berth.pos, item.pos);
                    auto robot_to_item = distance(exec->pos, item.pos);
                    if(robot_to_item > live_time) { continue; }
                    auto update = [&](auto x, auto y) {
                        if(x >= dp.size()) { return; }
                        float rate = 1.f - (float) (live_time) / Item::OVERDUE;
                        rate = (float) std::pow(rate, 0.42);
                        float value = dp[y] + rate * (float) item.value;
                        if(value > dp[x]) {
                            dp[x] = value;
                            item_list[x] = item_list[y];
                            item_list[x].emplace_back(item.unique_id);
                        }
                    };
                    int max_t = live_time - berth_to_item;
                    for(int j = max_t; j > 0; j--) {
                        if(dp[j] == 0) { continue; }
                        update(j + 2 * berth_to_item, j);
                    }
                    update(robot_to_item + berth_to_item, 0);
                }
                int best_last_j = 0;
                float best_value = 0.f;
                for(int j = 1; j < dp.size(); j++) {
                    float value = dp[j] / (float) j;
                    if(value > best_value) {
                        best_value = value;
                        best_last_j = j;
                    }
                }
                if(best_value > mission.reserved_value) {
                    mission.reserved_value = best_value;
                    mission.targets.clear();
                    for(auto id: item_list[best_last_j]) {
                        mission.targets.emplace_back(id, berth.id);
                    }
                }
            }
            if(mission.targets.empty()) { return mission = idle; }
            for(auto [id, _]: mission.targets) {
                Items::items.find_by_id(id).occupied = exec;
            }
            return mission;
        }
#endif
#ifdef Robot_idea_4
        Mission create(decltype(executor) exec) {
            auto &mission = *this;
            if(!mission.vacant()) { return mission; }
            // if(mission.mission_state == CARRYING) { return mission; }
            mission = {SEARCHING, exec, 0.f};
            for(auto [id, _]: exec->mission.targets) {
                Items::items.find_by_id(id).occupied = nullptr;
            }
            auto distance = [](auto p1, auto p2) -> int { return Atlas::atlas.distance(p1, p2); };
            std::array<std::array<float, 3 * Item::OVERDUE>, BERTH_NUM> dp{};
            std::array<std::unordered_map<int, decltype(mission.targets)>, BERTH_NUM> item_list;
            for(auto &item: Items::items) {
                if(item.occupied) { continue; }
                auto live_time = item.live_time();
                auto robot_to_item = distance(exec->pos, item.pos);
                if(robot_to_item == Atlas::INF_DIS || robot_to_item > live_time) { continue; }

                auto g = dp;
                for(auto &to_berth: Berths::berths) {
                    if(to_berth.disabled_pulling || distance(exec->pos, to_berth.pos) == Atlas::INF_DIS) { continue; }
                    auto update = [&](auto x, auto i, auto j) {
                        if(x >= dp[to_berth.id].size()) { return; }
                        float rate = 1.f - (float) live_time / (float) Item::OVERDUE;
                        rate = (float) std::pow(rate, 0.42);
                        float value = g[i][j] + rate * (float) item.value;
                        if(value > dp[to_berth.id][x]) {
                            dp[to_berth.id][x] = value;
                            item_list[to_berth.id][x] = item_list[i][j];
                            item_list[to_berth.id][x].emplace_back(item.unique_id, to_berth.id);
                        }
                    };
                    auto to_berth_to_item = distance(to_berth.pos, item.pos);
                    for(auto &from_berth: Berths::berths) {
                        if(from_berth.disabled_pulling || distance(exec->pos, from_berth.pos) == Atlas::INF_DIS) { continue; }
                        auto from_berth_to_item = distance(from_berth.pos, item.pos);
                        for(int j = live_time - from_berth_to_item; j > 0; j--) {
                            if(g[from_berth.id][j] == 0) { continue; }
                            update(j + from_berth_to_item + to_berth_to_item, from_berth.id, j);
                        }
                    }
                    update(robot_to_item + to_berth_to_item, 0, 0);
                }
            }
            std::pair<index_t, int> best_idx;
            for(auto &berth: Berths::berths) {
                if(berth.disabled_pulling) { continue; }
                for(int j = 1; j < dp[berth.id].size(); j++) {
                    float value = dp[berth.id][j] / (float) j;
                    if(value > mission.reserved_value) {
                        mission.reserved_value = value;
                        best_idx = {berth.id, j};
                    }
                }
            }
            if(item_list[best_idx.first][best_idx.second].empty()) { return mission = idle; }
            // DEBUG if(item_list[best_idx.first][best_idx.second].size() > mission.targets.size()){
            //     std::cerr << "size: " << mission.targets.size() << ' ';
            //     std::cerr << "size: " << item_list[best_idx.first][best_idx.second].size() << '\n';
            // }
            mission.targets = item_list[best_idx.first][best_idx.second];
            for(auto [id, _]: mission.targets) {
                Items::items.find_by_id(id).occupied = exec;
            }
            return mission;
        }
#endif

        auto check_item_overdue() {
            if(mission_state == SEARCHING) {
                if(auto &item = Items::items.find_by_id(targets.front().first);
                   item.deleted || item.live_time() < Atlas::atlas.distance(executor->pos, item.pos)) {
                    targets.pop_front();
                    if(targets.empty()) {
                        mission_state = IDLING;
                        return true;
                    }
                    return false;
                }
            }
            return true;
        }
        auto check_complete() {
            if(Item &item = Items::items.find_by_id(targets.front().first);
               mission_state == CARRYING && !item.deleted &&
               executor && executor->goods && Berths::berths[targets.front().second].inside(executor->pos)) {
                Berths::berths[targets.front().second].sign(item);
                targets.pop_front();
                if(targets.empty()) {
                    mission_state = IDLING;
                } else {
                    mission_state = SEARCHING;
                }
            }
        }
        auto check_carry() {
            if(Item &item = Items::items.find_by_id(targets.front().first);
               item != Item::noItem &&
               mission_state == SEARCHING && executor && executor->goods && executor->pos == item.pos) {
                mission_state = CARRYING;
                Berths::berths[targets.front().second].notify(item);
            }
        }
        auto forward() {
            if(!executor) { return; }
            switch(mission_state) {
            case WAITING: {
                next_move = Position::npos;
            } break;
            case IDLING: {
                auto move = Atlas::atlas.around(executor->pos);
                next_move = move[eng() % move.size()];
            } break;
            case SEARCHING: {
                next_move = Atlas::atlas.path(executor->pos, Items::items.find_by_id(targets.front().first).pos);
            } break;
            case CARRYING: {
                next_move = Atlas::atlas.path(executor->pos, Berths::berths[targets.front().second].pos);
            } break;
            }
        }
    };
    index_t id;
    bool goods{};
    Position pos;
    Mission mission = Mission::idle;

    [[maybe_unused]] static constexpr int ROBOT_DISABLE_TIME = 20;

    friend auto &operator>>(std::istream &in, Robot &r) {
        return in >> r.id >> r.goods >> r.pos;
    }
};

struct Robots : public std::vector<Robot> {
    using vector::vector;
    static Robots robots;
    Robots() {
        reserve(20);
    }

    void new_robot(Position p) {
        auto id = static_cast<index_t>(size());
        prior.emplace_back(id);
        push_back({id, false, p, Robot::Mission::idle});
    }

    std::vector<index_t> prior{};

    const static std::set<char> ROBOT_MULTI_SYM;

    auto resolve() {
        return std::async(std::launch::async, [this] {
            for(auto &robot: *this) {
                robot.mission.next_move = Position::npos;
                while(!robot.mission.check_item_overdue()) {}
                robot.mission.check_complete();
                robot.mission.create(&robot);
                robot.mission.check_carry();
                robot.mission.forward();
            }

            std::function<bool()> obstacle_avoiding = [&] {
                std::vector<Position> best_move(size());
                std::map<Position, index_t> obstacles;
                for(int i = 0; i < size(); i++) {
                    auto &robot = this->operator[](prior[i]);
                    Position now = robot.pos, next_move = robot.mission.next_move;
                    if(obstacles.count(now + next_move)) {
                        for(const auto &move: Atlas::atlas.around(now)) {
                            if(!obstacles.count(now + move)) {
                                if(obstacles.count(now + next_move)) {
                                    next_move = move;
                                    continue;
                                }
                                if(robot.mission.mission_state == Robot::Mission::MISSION_STATE::SEARCHING) {
                                    auto &item = Items::items.find_by_id(robot.mission.targets.front().first);
                                    if(Atlas::atlas.distance(now + move, item.pos) <
                                       Atlas::atlas.distance(now + next_move, item.pos)) {
                                        next_move = move;
                                    }
                                } else if(robot.mission.mission_state == Robot::Mission::MISSION_STATE::CARRYING) {
                                    auto &berth = Berths::berths[robot.mission.targets.front().second];
                                    if(Atlas::atlas.distance(now + move, berth.pos) <
                                       Atlas::atlas.distance(now + next_move, berth.pos)) {
                                        next_move = move;
                                    }
                                }
                            }
                        }
                        if(obstacles.count(now + next_move)) {
                            return false;
                        }
                    }
                    if(!ROBOT_MULTI_SYM.count(Atlas::atlas.maze[now])) {
                        obstacles.insert({now, i});
                    }
                    if(!ROBOT_MULTI_SYM.count(Atlas::atlas.maze[now + next_move])) {
                        obstacles.insert({now + next_move, i});
                    }
                    best_move[prior[i]] = next_move;
                }
                for(int i = 0; i < size(); i++) {
                    this->operator[](i).mission.next_move = best_move[i];
                }
                return true;
            };
            while(!obstacle_avoiding()) {
                std::shuffle(prior.begin(), prior.end(), eng);
            }
        });
    }
};

/**
 * idea:
 * 1. 设定mission时找value最优的, 之后只做每次增量更新 (now)
 *  状态机模型: WAITING/IDLING -> SEARCHING; SEARCHING -> CARRYING; CARRYING -> IDLING
 *  Exception:
 *      1. 最差情况下1000*10都重置mission的复杂度较差
 *      2. 没有按泊位信息(运输时间、装载速度)优化
 *      3. 对Item做互斥占有, 目前是维护id二分查找
 *          (优化思路:items改为linked-list/circular-queue存储, 维护迭代器, 可以全On)
 *      4. 没有做避让机制 (next)
 *          4.1 [*已添加] 目前idea是维护robot的位置和前进位置的set, 然后判断若出现阻塞则随机换向/不动(除非自身位置也不保)
 *          4.2 暂时没有考虑每一帧做A*等搜索来实现 (时间似乎能够)
 *          4.3 [*已修复] 若避让或撞击导致抵达时间延期, 可能导致货物消失, 没有做货物消失的update/或选取时预留好误差
 *          4.4 Lamkappa: 采用shuffle优先级避障
 *      5. [*已修改] 其实运货不够快, 以至于装货都来不及, 所以不需要loading_speed调参
 * 2. 将货物挂在最近的码头, 用set维护 (时间优化)
 *  Exception:
 *      1. 货物太少, 需要换码头, 远的码头可能更差
 * 3. [*已添加] 注意到泊位附近的物品总是不太够的, 考虑物品生命周期进行调度, 对一个序列的物品做背包即可
 *      3.1 注意到不应该以单位路径价值比作为选取背包的依据, 背包的目的是进行时间上的调度, 使得物品的生命周期得到充分利用
 * 4. [*已添加] 对物品的调度, 考虑可以在不同泊位附近变换, 做一个高维背包
 * 5. [*已添加] 初始化调度到更优的港口 (计算期望价值 -> 有点不对)
 *
 * BUGS:
 * 1. [*已修复] 有时候机器人取到货物后在某个地方傻住不动, 原因:有些机器人路过把他的item拿了
 * 2. [稳定复现] segment-fault: map-3.12 seed=6 eng=1 Robot避障开启next_move=move
 * 3. 被拿起的item如果被杀死, notify的value会不正确 (无害)
 * 4. [*已解决] 避障算法在3个不同优先级的robot在一条线时还是会卡死
 * 5. 原create在互换berths与items的迭代顺序的情况下, 分数波动很大(seed=2, eng=42, map=-3.13)
 * */

#endif//CODECRAFTSDK_ROBOT_HPP
