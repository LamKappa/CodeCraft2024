#pragma once
#ifndef CODECRAFTSDK_ROBOT_HPP
#define CODECRAFTSDK_ROBOT_HPP

#include <array>
#include <future>
#include <istream>

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
            IDLING,
            SEARCHING,
            CARRYING,
        };
        MISSION_STATE mission_state{MISSION_STATE::IDLING};
        Robot *executor{nullptr};
        float reserved_value{0.f};
        std::array<Position, 2> target{Position::npos};
        Position next_move{Position::npos};

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
                for(auto &berth: Berths::berths) {
                    float value = calc_value(*exec, item, berth);
                    if(value > mission.reserved_value) {
                        mission.reserved_value = value;
                        mission.target = {item.pos, berth.pos};
                    }
                }
            }
            return mission;
        }

        [[nodiscard]] auto complete() {
            return mission_state == CARRYING &&
                   (executor && executor->pos == target[1]);
        }
        auto update() {
            for(auto itr = Items::items.rbegin(); itr != Items::items.rend() && itr->stamp == ::stamp; itr++) {
                auto &item = *itr;
                for(auto &berth: Berths::berths) {
                    float value = calc_value(*executor, item, berth);
                    if(value > reserved_value) {
                        reserved_value = value;
                        target = {item.pos, berth.pos};
                    }
                }
            }
        }
        auto forward() {
            if(!executor) { return; }
            if(mission_state == SEARCHING && executor && executor->goods) {
                mission_state = CARRYING;
            }
            switch(mission_state) {
            case IDLING: {
                /* todo */
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
            for(auto &robot: robots) {
                if(robot.mission.complete()) {
                    robot.mission = Robot::Mission::create(&robot);
                } else {
                    robot.mission.update();
                }
                robot.mission.forward();
            }
        });
    }
};

#endif//CODECRAFTSDK_ROBOT_HPP
