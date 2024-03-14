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
        MISSION_STATE mission_state{MISSION_STATE::IDLING};
        Robot *executor{nullptr};
        float reserved_value{0.f};
        std::pair<long, index_t> target{Item::noItem.unique_id, no_index};
        Position next_move{Position::npos};

        static Mission idle;
        [[nodiscard]] static auto calc_value(const Robot &robot, const Item &item, const Berth &berth) {
            // item.value / distance(robot -> item -> berth)

            Atlas &atlas = Atlas::atlas;
            // value / (2*tans + cap / ld_t + dis)
            float value = (float) (item.value) /
                          (float) ((float) atlas.distance(robot.pos, item.pos) +
                                   (float) atlas.distance(item.pos, berth.pos));
            return value;
        }
        static Mission create(decltype(executor) exec) {
            Mission mission = {SEARCHING, exec};
            for(auto &item: Items::items) {
                if(item.occupied) { continue; }
                if(item.live_time() < Atlas::atlas.distance(exec->pos, item.pos)) { continue; }
                for(auto &berth: Berths::berths) {
                    if(berth.disabled || Atlas::atlas.distance(exec->pos, berth.pos) == Atlas::INF_DIS) { continue; }
                    float value = calc_value(*exec, item, berth);
                    if(value > mission.reserved_value) {
                        if(mission.target.first != Item::noItem.unique_id) {
                            Items::items.find_by_id(mission.target.first).occupied = false;
                        }
                        item.occupied = true;
                        mission.reserved_value = value;
                        mission.target = {item.unique_id, berth.id};
                    }
                }
            }
            if(mission.reserved_value <= 0.f) { return idle; }
            return mission;
        }

        [[nodiscard]] inline auto vacant() const {
            return mission_state == WAITING || mission_state == IDLING;
        }
        auto check_item_overdue() {
            if(auto &item = Items::items.find_by_id(target.first);
               mission_state == SEARCHING &&
               item.live_time() < Atlas::atlas.distance(executor->pos, item.pos)) {
                mission_state = IDLING;
            }
        }
        auto check_complete() {
            if(mission_state == CARRYING &&
               executor && Berths::berths[target.second].inside(executor->pos)) {
                Berths::berths[target.second]
                        .sign(Items::items.find_by_id(target.first).value);
                mission_state = IDLING;
            }
        }
        auto update() {
            if(mission_state != SEARCHING) { return; }
            for(auto itr = Items::items.rbegin(); itr != Items::items.rend() && itr->stamp == ::stamp; itr++) {
                auto &item = *itr;
                if(item.occupied) { continue; }
                for(auto &berth: Berths::berths) {
                    if(berth.disabled || Atlas::atlas.distance(executor->pos, berth.pos) == Atlas::INF_DIS) { continue; }
                    float value = calc_value(*executor, item, berth);
                    if(value > reserved_value) {
                        if(target.first != Item::noItem.unique_id) {
                            Items::items.find_by_id(target.first).occupied = false;
                        }
                        item.occupied = true;
                        reserved_value = value;
                        target = {item.unique_id, berth.id};
                    }
                }
            }
        }
        auto check_carry() {
            if(mission_state == SEARCHING &&
               (executor && executor->goods)) {
                mission_state = CARRYING;
                Berths::berths[target.second]
                        .notify(Items::items.find_by_id(target.first).value);
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
                next_move = Atlas::atlas.path(executor->pos, Items::items.find_by_id(target.first).pos);
            } break;
            case CARRYING: {
                next_move = Atlas::atlas.path(executor->pos, Berths::berths[target.second].pos);
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
                robot.mission.update();
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
            while(!obstacle_avoiding())
                ;
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
 *
 * BUGS:
 * 1. [*已修复] 有时候机器人取到货物后在某个地方傻住不动, 原因:有些机器人路过把他的item拿了
 * 2. [稳定复现] segment-fault: map-3.12 seed=6 eng=1 Robot避障开启next_move=move
 * 3. 被拿起的item如果被杀死, notify的value会不正确 (无害)
 * 4. [*已解决] 避障算法在3个不同优先级的robot在一条线时还是会卡死
 * */

#endif//CODECRAFTSDK_ROBOT_HPP
