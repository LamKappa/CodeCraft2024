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
            WAITTING,
            IDLING,
            SEARCHING,
            CARRYING,
        };
        MISSION_STATE mission_state{MISSION_STATE::WAITTING};
        Robot *executor{nullptr};
        float reserved_value{0.f};
        std::array<Position, 2> target{Position::npos};
        Position next_move{Position::npos};
        long item_id = -1;

        static Mission idle;
        [[nodiscard]] static auto calc_value(const Robot &robot, const Item &item, const Berth &berth) {
            // item.value / distance(robot -> item -> berth)
            Atlas &atlas = Atlas::atlas;
            float value = (float) item.value /
                          (float) (atlas.distance(robot.pos, item.pos) + atlas.distance(item.pos, berth.pos));
            return value;
        }
        static Mission create(decltype(executor) exec) {
            Mission mission = {SEARCHING, exec};
            for(auto &item: Items::items) {
                if(item.occupied) { continue; }
                for(auto &berth: Berths::berths) {
                    float value = calc_value(*exec, item, berth);
                    if(value > mission.reserved_value) {
                        if(mission.item_id >= 0){
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
            return mission_state == WAITTING || mission_state == IDLING;
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
//            if(mission_state != SEARCHING) { return; }
//            for(auto itr = Items::items.rbegin(); itr != Items::items.rend() && itr->stamp == ::stamp; itr++) {
//                auto &item = *itr;
//                if(item.occupied) { continue; }
//                for(auto &berth: Berths::berths) {
//                    float value = calc_value(*executor, item, berth);
//                    if(value > reserved_value) {
//                        item_value = item.value;
//                        reserved_value = value;
//                        target = {item.pos, berth.pos};
//                    }
//                }
//            }
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
            case WAITTING: {
            } break;
            case IDLING: {
                next_move = Move[eng() % 4];
            } break;
            case SEARCHING: {
                next_move = Atlas::atlas.path(executor->pos, target[0]);
            } break;
            case CARRYING: {
                next_move = Atlas::atlas.path(executor->pos, target[1]);
                if(next_move == Position::npos){
                    Flatten_Position A = executor->pos, B = target[1];
                    if(A > B) std::swap(A, B);
                    std::cerr << "from " << executor->pos << " to " << target[1]
                              << " dis(" << (1 + A) * A / 2 + B << ") = " << Atlas::atlas.dist[(1 + A) * A / 2 + B]
                              << " dis:" << Atlas::atlas.distance(executor->pos, target[1]) << std::endl;
                }
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
            for(auto &robot: robots) {
                robot.mission.check_complete();
                if(robot.mission.vaccant()) {
                    robot.mission = Robot::Mission::create(&robot);
                } else {
                    robot.mission.update();
                }
                robot.mission.check_carry();
                robot.mission.forward();
            }
        });
    }
};

/**
 * idea:
 * 1. 设定mission时找value最优的, 之后只做每次增量更新 (now)
 *  Exception:
 *      1. 最差情况下1000*10都重置mission的复杂度较差
 * 2. 将货物挂在最近的码头, 用set维护 (时间优化)
 *  Exception:
 *      1. 货物太少, 需要换码头
 * */

#endif//CODECRAFTSDK_ROBOT_HPP
