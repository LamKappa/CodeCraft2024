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
        MISSION_STATE mission_state{MISSION_STATE::WAITING};
        Robot *executor{nullptr};
        float reserved_value{0.f};
        std::array<Position, 2> target{Position::npos};
        Position next_move{Position::npos};
        long item_id = -1;

        static Mission idle;
        [[nodiscard]] static auto calc_value(const Robot &robot, const Item &item, const Berth &berth) {
            // item.value / distance(robot -> item -> berth)
            Atlas &atlas = Atlas::atlas;
            float value = (float) (item.value) /
                          (float) (atlas.distance(robot.pos, item.pos) + atlas.distance(item.pos, berth.pos));
            return value;
        }
        static Mission create(decltype(executor) exec) {
            Mission mission = {SEARCHING, exec};
            for(auto &item: Items::items) {
                if(item.occupied) { continue; }
                if(item.live_time() < Atlas::atlas.distance(exec->pos, item.pos)) { continue; }
                for(auto &berth: Berths::berths) {
                    if(berth.disabled) { continue; }
                    float value = calc_value(*exec, item, berth);
                    if(value > mission.reserved_value) {
                        if(mission.item_id >= 0) {
                            Items::find_by_id(mission.item_id).occupied = false;
                        }
                        item.occupied = true;
                        mission.item_id = item.unique_id;
                        mission.reserved_value = value;
                        mission.target = {item.pos, berth.pos};
                    }
                }
            }
            if(mission.reserved_value <= 0.f) { return idle; }
            return mission;
        }

        [[nodiscard]] inline auto vaccant() const {
            return mission_state == WAITING || mission_state == IDLING;
        }
        auto check_item_overdue() {
            if(mission_state == SEARCHING &&
               Items::items.find_by_id(item_id).live_time() < Atlas::atlas.distance(executor->pos, target[0])) {
                mission_state = IDLING;
            }
        }
        auto check_complete() {
            if(mission_state == CARRYING &&
               (executor && !executor->goods)) {
                Berths::berths.find_by_pos(target[1])
                        .sign(Items::find_by_id(item_id).value);
                mission_state = IDLING;
            }
        }
        auto update() {
            if(mission_state != SEARCHING) { return; }
            for(auto itr = Items::items.rbegin(); itr != Items::items.rend() && itr->stamp == ::stamp; itr++) {
                auto &item = *itr;
                if(item.occupied) { continue; }
                for(auto &berth: Berths::berths) {
                    if(berth.disabled) { continue; }
                    float value = calc_value(*executor, item, berth);
                    if(value > reserved_value) {
                        if(item_id >= 0) {
                            Items::find_by_id(item_id).occupied = false;
                        }
                        item.occupied = true;
                        item_id = item.unique_id;
                        reserved_value = value;
                        target = {item.pos, berth.pos};
                    }
                }
            }
        }
        auto check_carry() {
            if(mission_state == SEARCHING &&
               (executor && executor->goods)) {
                mission_state = CARRYING;
                Berths::berths.find_by_pos(target[1])
                        .notify(Atlas::atlas.distance(executor->pos, target[1]));
            }
        }
        auto forward() {
            if(!executor) { return; }
            switch(mission_state) {
            case WAITING: {
                next_move = Position::npos;
            } break;
            case IDLING: {
                next_move = Atlas::atlas.around(executor->pos);
            } break;
            case SEARCHING: {
                next_move = Atlas::atlas.path(executor->pos, target[0]);
            } break;
            case CARRYING: {
                next_move = Atlas::atlas.path(executor->pos, target[1]);
            } break;
            }
        }
    };
    Position pos;
    Mission mission;

    bool goods{};
    bool status{};

    friend auto &operator>>(std::istream &in, Robot &r) {
        return in >> r.goods >> r.pos >> r.status;
    }
};

struct Robots : public std::array<Robot, ROBOT_NUM> {
    using array::array;
    static Robots robots;
    Robots() = default;

    std::future<void> resolve() {
        return std::async(std::launch::async, [this] {
            std::set<Position> obstacles;
            auto obstacle_avoiding = [this, &obstacles](Robot &robot) {
                Position &now = robot.pos;
                Position &next_move = robot.mission.next_move;
                if(obstacles.count(now + next_move)) {
                    if(!obstacles.count(now)) {
                        next_move = Position::npos;
                    } else {
                        for(auto &move: Move) {
                            if(Atlas::atlas.bitmap.test(now + move)) { continue; }
                            if(!obstacles.count(now + move)) {
                                next_move = move;
                                break;
                            }
                        }
                    }
                }
                obstacles.insert(now);
                obstacles.insert(now + next_move);
            };
            for(auto &robot: robots) {
                robot.mission.check_item_overdue();
                robot.mission.check_complete();
                if(robot.mission.vaccant()) {
                    robot.mission = Robot::Mission::create(&robot);
                } else {
                    robot.mission.update();
                }
                robot.mission.check_carry();
                robot.mission.forward();
                obstacle_avoiding(robot);
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
 * 2. 将货物挂在最近的码头, 用set维护 (时间优化)
 *  Exception:
 *      1. 货物太少, 需要换码头, 远的码头可能更差
 *
 * BUGS:
 * 1. 有时候机器人取到货物后在某个地方傻住不动
 * */

#endif//CODECRAFTSDK_ROBOT_HPP
