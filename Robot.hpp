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

        static Mission create(decltype(executor) exec) {
            Mission mission = {SEARCHING, exec};
            auto calc_value = [](const Robot &robot, const Item &item, const Berth &berth) {
                Atlas &atlas = Atlas::atlas;
                return (float) (item.value) /
                       ((float) atlas.distance(robot.pos, item.pos) +
                        (float) atlas.distance(item.pos, berth.pos));
            };
            for(auto &berth: Berths::berths) {
                if(berth.disabled || Atlas::atlas.distance(exec->pos, berth.pos) == Atlas::INF_DIS) { continue; }
                for(auto &item: Items::items) {
                    if(item.occupied || item.live_time() < Atlas::atlas.distance(exec->pos, item.pos)) { continue; }
                    float value = calc_value(*exec, item, berth);
                    if(value > mission.reserved_value) {
                        mission.reserved_value = value;
                        mission.targets.assign(1, {item.unique_id, berth.id});
                    }
                }
            }
            if(mission.targets.empty()) { return idle; }
            for(auto [id, _]: mission.targets) {
                Items::items.find_by_id(id).occupied = true;
            }
            return mission;
        }

        // static Mission create(decltype(executor) exec) {
        //     Mission mission{SEARCHING, exec, 0.f};
        //     auto distance = [](auto p1, auto p2) -> int { return Atlas::atlas.distance(p1, p2); };
        //     for(auto &berth: Berths::berths) {
        //         if(berth.disabled || distance(exec->pos, berth.pos) == Atlas::INF_DIS) { continue; }
        //         std::vector dp(3 * Item::OVERDUE, 0);
        //         std::unordered_map<int, long> last_item;
        //         for(auto &item: Items::items) {
        //             if(item.occupied) { continue; }
        //             auto update = [&item, &dp, &last_item](auto x, auto y) {
        //                 if(x >= dp.size()) { return; }
        //                 if(dp[y] + item.value > dp[x]) {
        //                     dp[x] = dp[y] + item.value;
        //                     last_item[x] = item.unique_id;
        //                 }
        //             };
        //             auto live_time = item.live_time();
        //             auto berth_to_item = distance(berth.pos, item.pos);
        //             auto robot_to_item = distance(exec->pos, item.pos);
        //             int max_t = live_time - berth_to_item;
        //             for(int j = max_t; j > 0; j--) {
        //                 if(dp[j] == 0) { continue; }
        //                 update(j + 2 * berth_to_item, j);
        //             }
        //             if(robot_to_item > live_time) { continue; }
        //             update(robot_to_item + berth_to_item, 0);
        //         }
        //         int best_last_j = 0;
        //         float best_value = 0.f;
        //         for(int j = 1; j < dp.size(); j++) {
        //             float value = (float) dp[j] / (float) j;
        //             if(value > best_value) {
        //                 best_value = value;
        //                 best_last_j = j;
        //             }
        //         }
        //         if(best_value > mission.reserved_value) {
        //             mission.reserved_value = best_value;
        //             mission.targets.clear();
        //             while(best_last_j > 0 && last_item[best_last_j] != Item::noItem.unique_id) {
        //                 auto &item = Items::items.find_by_id(last_item[best_last_j]);
        //                 auto berth_to_item = distance(berth.pos, item.pos);
        //                 auto robot_to_item = distance(exec->pos, item.pos);
        //                 mission.targets.emplace_front(item.unique_id, berth.id);
        //                 if(best_last_j == berth_to_item + robot_to_item) { break; }
        //                 best_last_j -= 2 * berth_to_item;
        //             }
        //         }
        //     }
        //     if(mission.targets.empty()) { return idle; }
        //     std::cerr << "targets.size(): " << mission.targets.size() << '\n';
        //     for(auto [id, _]: mission.targets) {
        //         Items::items.find_by_id(id).occupied = true;
        //     }
        //     return mission;
        // }

        [[nodiscard]] inline auto vacant() const {
            return mission_state == WAITING || mission_state == IDLING;
        }
        void check_item_overdue() {
            if(mission_state != SEARCHING) { return; }
            if(auto &item = Items::items.find_by_id(targets.front().first);
               item.live_time() < Atlas::atlas.distance(executor->pos, item.pos)) {
                targets.pop_front();
                if(targets.empty()) {
                    mission_state = IDLING;
                } else {
                    check_item_overdue();
                }
            }
        }
        auto check_complete() {
            if(mission_state == CARRYING &&
               executor && Berths::berths[targets.front().second].inside(executor->pos)) {
                Berths::berths[targets.front().second]
                        .sign(Items::items.find_by_id(targets.front().first));
                targets.pop_front();
                if(targets.empty()) {
                    mission_state = IDLING;
                }
            }
        }
        auto check_carry() {
            if(mission_state == SEARCHING &&
               (executor && executor->goods)) {
                mission_state = CARRYING;
                Berths::berths[targets.front().second]
                        .notify(Items::items.find_by_id(targets.front().first));
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
    Position pos;
    Mission mission;

    bool goods{};
    bool status{};

    [[maybe_unused]] static constexpr int ROBOT_DISABLE_TIME = 20;

    friend auto &operator>>(std::istream &in, Robot &r) {
        return in >> r.goods >> r.pos >> r.status;
    }
};

struct Robots : public std::array<Robot, ROBOT_NUM> {
    using array::array;
    static Robots robots;
    Robots() {
        std::iota(prior.begin(), prior.end(), 0);
    };

    std::array<index_t, ROBOT_NUM> prior{};

    auto init() {
        // for(auto &berth: Berths::berths) {
        //     Robot::Mission mission{
        //             Robot::Mission::MISSION_STATE::CARRYING,
        //             nullptr,
        //             0,
        //             {Item::noItem.unique_id, berth.id},
        //             Position::npos};
        //     mission.executor = std::min_element(begin(), end(), [&berth](auto&x, auto&y){
        //         if(x.mission.mission_state == Robot::Mission::MISSION_STATE::CARRYING || y.mission.mission_state == Robot::Mission::MISSION_STATE::CARRYING){
        //             return y.mission.mission_state == Robot::Mission::MISSION_STATE::CARRYING;
        //         }
        //         return Atlas::atlas.distance(x.pos, berth.pos) < Atlas::atlas.distance(y.pos, berth.pos);
        //     });
        //     mission.executor->mission = mission;
        // }
    }

    auto resolve() {
        return std::async(std::launch::async, [this] {
            for(auto &robot: robots) {
                robot.mission.next_move = Position::npos;
                robot.mission.check_item_overdue();
                robot.mission.check_complete();
                if(robot.mission.vacant()) {
                    robot.mission = Robot::Mission::create(&robot);
                }
                robot.mission.check_carry();
                robot.mission.forward();
            }

            std::function<bool()> obstacle_avoiding = [&] {
                std::array<Position, ROBOT_NUM> best_move;
                std::map<Position, index_t> obstacles;
                for(int i = 0; i < ROBOT_NUM; i++) {
                    auto &robot = robots[prior[i]];
                    Position now = robot.pos, next_move = robot.mission.next_move;
                    if(obstacles.count(now + next_move)) {
                        for(const auto &move: Atlas::atlas.around(now)) {
                            if(!obstacles.count(now + move)) {
                                next_move = move;
                                break;
                            }
                        }
                        if(obstacles.count(now + next_move)) {
                            std::shuffle(prior.begin(), prior.end(), eng);
                            return false;
                        }
                    }
                    obstacles.insert({now, i});
                    obstacles.insert({now + next_move, i});
                    best_move[prior[i]] = next_move;
                }
                for(int i = 0; i < ROBOT_NUM; i++) {
                    robots[i].mission.next_move = best_move[i];
                }
                return true;
            };
            while(!obstacle_avoiding()) {}
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
 *
 * BUGS:
 * 1. [*已修复] 有时候机器人取到货物后在某个地方傻住不动, 原因:有些机器人路过把他的item拿了
 * 2. [稳定复现] segment-fault: map-3.12 seed=6 eng=1 Robot避障开启next_move=move
 * 3. 被拿起的item如果被杀死, notify的value会不正确 (无害)
 * 4. [*已解决] 避障算法在3个不同优先级的robot在一条线时还是会卡死
 * 5. 原create在互换berths与items的迭代顺序的情况下, 分数波动很大(seed=2, eng=42, map=-3.13)
 * */

#endif//CODECRAFTSDK_ROBOT_HPP
