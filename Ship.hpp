#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>
#include <map>

#include "Berth.hpp"
#include "Config.h"

struct Ship {
    int load = 0;
    int status{};
    index_t berth_id{};
    bool sail_out = false;

    struct Mission {
        Mission() = default;
        enum MISSION_STATE {
            WAITING,
            SAILING,
            LOADING,
            QUEUEING,
        };
        MISSION_STATE mission_state{MISSION_STATE::WAITING};
        Ship *executor{nullptr};
        index_t target = no_index;

        static Mission create(decltype(executor) exec, index_t target) {
            return Mission{SAILING, exec, target};
        }
        auto check_waiting() {
            if(!executor) { return; }
            if(executor->status == 1 && executor->berth_id == no_index) {
                mission_state = WAITING;
                executor->sail_out = false;
                executor->load = 0;
                executor->status = 3;
            }
        }
        auto check_loading() {
            if(!executor) { return; }
            if(executor->status == 1 && executor->berth_id != no_index) {
                mission_state = LOADING;
                executor->sail_out = false;
                executor->status = 1;
            }
        }
        auto check_queueing() {
            if(!executor) { return; }
            if(executor->status == 2 && executor->berth_id != no_index) {
                mission_state = QUEUEING;
                executor->sail_out = false;
                executor->status = 1;
            }
        }
        auto check_overload() {
            if(!executor) { return; }
            if(mission_state != LOADING) { return; }
            if(executor->load >= SHIP_CAPACITY) {
                mission_state = SAILING;
                target = no_index;
            }
        }
        auto forward() {
            if(!executor) { return; }
            switch(mission_state) {
            case WAITING: {
            } break;
            case SAILING: {
            } break;
            case LOADING: {
                executor->load++;
                Berths::berths[executor->berth_id].get_load();
            } break;
            case QUEUEING: {
            } break;
            }
        }
    };
    Mission mission{};

    Ship() = default;

    friend auto &operator>>(std::istream &in, Ship &b) {
        return in >> b.status >> b.berth_id;
    }
};

struct Ships : public std::array<Ship, SHIP_NUM> {
    using array::array;
    static Ships ships;
    Ships() = default;

    static std::map<index_t, std::reference_wrapper<const std::function<void()>>> waitlist;

    static auto wanted(index_t berth_id, const std::function<void()> &recall) {
        for(int i = 0; i < SHIP_NUM; i++) {
            auto &ship = ships[i];
            if(ship.mission.mission_state != Ship::Mission::MISSION_STATE::WAITING) { continue; }
            ship.mission = Ship::Mission::create(&ship, berth_id);
            ship.status = 3;
            return (void) recall();
        }
        waitlist.insert({berth_id, recall});
    }

    std::future<void> resolve() {
        return std::async(std::launch::async, [this] {
            for(auto itr = waitlist.begin(); itr != waitlist.end();) {
                auto [berth_id, recall] = *itr++;
                wanted(berth_id, [itr, recall] {
                    recall.get()();
                    waitlist.erase(itr);
                });
            }
            for(auto &ship: ships) {
                ship.mission.check_waiting();
                ship.mission.check_loading();
                ship.mission.check_queueing();
                ship.mission.forward();
                ship.mission.check_overload();
            }
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
 * */


#endif//CODECRAFTSDK_SHIP_HPP
