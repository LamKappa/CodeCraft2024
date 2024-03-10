#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>

#include "Berth.hpp"
#include "Config.h"

struct Ship {
    int load = 0;
    int status{};
    index_t berth_id{};
    static int CAPACITY;

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
                executor->load = 0;
                executor->status = 3;
            }
        }
        auto check_overload() {
            if(!executor) { return; }
            if(mission_state != LOADING) { return; }
            if(executor->load >= CAPACITY) {
                mission_state = SAILING;
                target = no_index;
            }
        }
        auto forward() {
            if(!executor) { return; }
            switch(executor->status) {
            case 0: mission_state = SAILING; break;
            case 1: mission_state = LOADING; break;
            case 2: mission_state = QUEUEING; break;
            }
            switch(mission_state) {
            case WAITING: {
            } break;
            case SAILING: {
            } break;
            case LOADING: {
                executor->load += Berths::berths[executor->berth_id].get_load();
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

    static auto wanted(index_t berth_id) {
        for(int i = 0; i < SHIP_NUM; i++) {
            auto &ship = ships[i];
            if(ship.mission.mission_state != Ship::Mission::MISSION_STATE::WAITING) { continue; }
            ship.mission = Ship::Mission::create(&ship, berth_id);
            ship.status = 3;
            return (index_t) i;
        }
        return no_index;
    }

    std::future<void> resolve() {
        return std::async(std::launch::async, [this] {
            for(auto &ship: ships) {
                ship.mission.check_waiting();
                ship.mission.forward();
                ship.mission.check_overload();
            }
        });
    }
};

#endif//CODECRAFTSDK_SHIP_HPP
