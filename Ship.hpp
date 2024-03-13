#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>
#include <set>

#include "Berth.hpp"
#include "Config.h"

struct Ship {
    int load = 0;
    int status{};
    index_t berth_id = no_index;
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
            Berths::berths[target].occupied++;
            return Mission{SAILING, exec, target};
        }
        auto check_arrival() const {
            if(!executor) { return; }
            if(executor->status != 0) {
                executor->sail_out = false;
            }
        }
        auto check_waiting() {
            if(!executor) { return; }
            if(executor->status == 1 && executor->berth_id == no_index && target == no_index) {
                mission_state = WAITING;
                executor->load = 0;
                executor->status = 3;
            }
        }
        auto check_loading() {
            if(!executor) { return; }
            if(executor->status == 1 && executor->berth_id == target && target != no_index) {
                mission_state = LOADING;
                executor->status = 1;
            }
        }
        auto check_queueing() {
            if(!executor) { return; }
            if(executor->status == 2 && executor->berth_id != target && target != no_index) {
                mission_state = QUEUEING;
                executor->status = 1;
            }
        }
        auto check_overload() {
            if(!executor) { return; }
            if(mission_state != LOADING) { return; }
            if(executor->load == SHIP_CAPACITY) {
                mission_state = SAILING;
                target = no_index;
                Berths::berths[executor->berth_id].occupied--;
            }
        }
        auto forward() const {
            if(!executor) { return; }
            switch(mission_state) {
            case WAITING: {
            } break;
            case SAILING: {
            } break;
            case LOADING: {
                auto [cnt, value] = Berths::berths[executor->berth_id].get_load(SHIP_CAPACITY - executor->load);
                executor->load += cnt;
                // std::cerr << "need " << (SHIP_CAPACITY - executor->load) << " get " << cnt << std::endl;
            } break;
            case QUEUEING: {
            } break;
            }
        }
    };
    Mission mission{};

    Ship() = default;

    static auto transport(index_t s, index_t t) {
        if(s != no_index && t != no_index) {
            return Berth::TRANSPORT_TIME < Berths::berths[s].transport_time + Berths::berths[t].transport_time ? t : no_index;
        }
        std::array<index_t, BERTH_NUM> rk{};
        std::iota(rk.begin(), rk.end(), 0);
        auto calc = [&](int i) {
            return (i == s || i == t ? 0 : Berth::TRANSPORT_TIME) + Berths::berths[i].transport_time;
        };
        auto best_i = *std::min_element(rk.begin(), rk.end(), [&calc](auto i, auto j) {
            return calc(i) < calc(j);
        });
        return best_i == s ? t : best_i;
    }

    friend auto &operator>>(std::istream &in, Ship &b) {
        return in >> b.status >> b.berth_id;
    }
};

struct Ships : public std::array<Ship, SHIP_NUM> {
    using array::array;
    static Ships ships;
    Ships() = default;

    static std::set<index_t> waitlist;

    static auto wanted(index_t berth_id) {
        for(int i = 0; i < SHIP_NUM; i++) {
            auto &ship = ships[i];
            if(ship.mission.mission_state != Ship::Mission::MISSION_STATE::WAITING) { continue; }
            ship.mission = Ship::Mission::create(&ship, berth_id);
            ship.status = 3;
            return waitlist.erase(berth_id), true;
        }
        return waitlist.insert(berth_id), false;
    }

    auto resolve() {
        return std::async(std::launch::async, [this] {
            // auto waitlist_copy = waitlist;
            // for(auto berth_id : waitlist_copy){
            //     wanted(berth_id);
            // }
            for(auto &ship: ships) {
                ship.mission.check_arrival();
                ship.mission.check_waiting();
                ship.mission.check_loading();
                ship.mission.check_queueing();
                ship.mission.check_overload();
                ship.mission.forward();
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
 * 2. transport, 用于计算从s到t的最优寻路, 此情景下最多中转一次
 * BUGS:
 *  1. [*已解决] Ship每次load最多loading_speed个货物, 但是要看berth够不够
 * */


#endif//CODECRAFTSDK_SHIP_HPP
