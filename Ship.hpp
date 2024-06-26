#pragma once
#ifndef CODECRAFTSDK_SHIP_HPP
#define CODECRAFTSDK_SHIP_HPP

#include <array>
#include <future>
#include <istream>
#include <set>

#include "Berth.hpp"
#include "Config.h"

#define Ship_idea_4

struct Ship {
    int load = 0;
    int load_values = 0;
    int status{};
    index_t berth_id = no_index;
    bool sail_out = false;

    static auto transport(index_t s, index_t t) {
        static std::map<std::pair<index_t, index_t>, std::pair<int, index_t>> transport_record;
        if(transport_record.count({s, t})) {
            return transport_record[{s, t}];
        }
        if(s == t) { return std::make_pair(0, s); }
        if(s != no_index && t != no_index) {
            return transport_record[{s, t}] = std::min<std::pair<int, index_t>>(
                           {Berth::TRANSPORT_TIME, t},
                           {Berths::berths[s].transport_time + Berths::berths[t].transport_time, no_index});
        }
        std::array<index_t, BERTH_NUM> rk{};
        std::iota(rk.begin(), rk.end(), 0);
        auto calc = [&](int i) {
            return (i == s || i == t ? 0 : Berth::TRANSPORT_TIME) + Berths::berths[i].transport_time;
        };
        auto best_i = *std::min_element(rk.begin(), rk.end(), [&calc](auto i, auto j) {
            return calc(i) < calc(j);
        });
        return transport_record[{s, t}] = std::make_pair(calc(best_i), best_i == s ? t : best_i);
    }

    struct Mission {
        Mission() = default;
        enum MISSION_STATE {
            WAITING,
            SAILING,
            LOADING,
            QUEUEING,
        };
        static Mission waiting;

        MISSION_STATE mission_state{MISSION_STATE::WAITING};
        Ship *executor{nullptr};
        float reserved_value = 0.f;
        index_t target = no_index;
        index_t next_move = no_index;

        [[nodiscard]] auto vacant() const {
            return mission_state == WAITING;
        }
#ifdef Ship_idea_3
        static Mission create(decltype(executor) exec) {
            static const float NOT_VALUABLE = 0.f;
            if(exec->mission.mission_state == SAILING) { return exec->mission; }
            Berths::berths[exec->berth_id].occupied--;
            Mission mission = {SAILING, exec, 0.f, exec->berth_id};
            for(auto &berth: Berths::berths) {
                if(berth.disabled_loading || berth.occupied) { continue; }
                auto time = transport(exec->berth_id, berth.id).first + transport(berth.id, no_index).first;
                if(stamp + time > MAX_FRAME) { continue; }
                // int berth_hold = berth.notified + (int) berth.cargo.size();
                int berth_hold_value = berth.notified_value + berth.cargo_value;
                auto value = (float) berth_hold_value;// + (float) berth_hold / (float) berth.loading_speed;
                if(value >= mission.reserved_value) {
                    mission.reserved_value = value;
                    mission.target = berth.id;
                }
            }
            if(exec->berth_id != no_index) {
                if(mission.target == exec->berth_id || mission.reserved_value < NOT_VALUABLE) {
                    mission = exec->mission;
                }
            } else if(mission.target == no_index) {
                return waiting;
            }
            Berths::berths[mission.target].occupied++;
            return mission;
        }
#endif
#ifdef Ship_idea_4
        Mission create(decltype(executor) exec) {
            auto &mission = *this;
            if(mission.mission_state == SAILING) { return mission; }
            Berths::berths[exec->berth_id].occupied = nullptr;
            mission = {SAILING, exec, 0.f, exec->berth_id};
            for(auto &berth: Berths::berths) {
                if(berth.disabled_loading) { continue; }
                auto time1 = transport(exec->berth_id, berth.id).first;
                auto time2 = transport(berth.id, no_index).first;
                if(stamp + time1 + time2 > MAX_FRAME) { continue; }
                int berth_hold = berth.notified + (int) berth.cargo.size();

                auto calc_value = [&berth, &berth_hold](float load_cnt) {
                    load_cnt = std::min(load_cnt, (float) berth_hold);
                    float load_value = 0;
                    for(int i = 0; i < std::min<int>((int) berth.cargo.size(), (int) load_cnt); i++) {
                        load_value += (float) berth.cargo[i].value;
                    }
                    if((int) load_cnt > (int) berth.cargo.size()) {
                        load_value += (float) berth.notified_value * (load_cnt - (float) berth.cargo.size()) / (float) berth.notified;
                    }
                    return load_value;
                };

                float load_time = (float) (MAX_FRAME - stamp - time1 - time2),
                      load_cnt = 0, load_value = 0;
                if(berth.occupied != nullptr && berth.occupied != exec) {
                    auto &ship = *static_cast<Ship *>(berth.occupied);
                    if((SHIP_CAPACITY - ship.load - 1) / berth.loading_speed + 1 > time1) {
                        continue;
                    } else {
                        load_value -= calc_value((float) (SHIP_CAPACITY - ship.load));
                        load_cnt += (float) (SHIP_CAPACITY - ship.load);
                    }
                    berth_hold = std::max(0, berth_hold - SHIP_CAPACITY + ship.load);
                }
                load_time = std::min(load_time, (float) berth_hold / (float) berth.loading_speed);
                load_cnt += load_time * (float) berth.loading_speed;
                load_value += calc_value(load_cnt);
                float value = load_value / (load_time + (float) time1);
                if(value >= mission.reserved_value) {
                    mission.reserved_value = value;
                    mission.target = berth.id;
                }
            }
            if(exec->berth_id != no_index) {
                if(mission.target == exec->berth_id) {
                    mission.mission_state = LOADING;
                }
            } else if(mission.target == no_index) {
                return mission = waiting;
            }
            if(Berths::berths[mission.target].occupied == nullptr) {
                Berths::berths[mission.target].occupied = exec;
            }
            return mission;
        }
#endif

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
                executor->status = 3;
            }
        }
        auto check_loading() {
            if(!executor) { return; }
            if(executor->status == 1 && executor->berth_id == target && target != no_index) {
                mission_state = LOADING;
                executor->status = 1;
                Berths::berths[executor->berth_id].occupied = executor;
            }
        }
        auto check_queueing() {
            if(!executor) { return; }
            if(executor->status == 2 && executor->berth_id == target && target != no_index) {
                mission_state = QUEUEING;
                executor->status = 1;
            }
        }
        auto check_overload() {
            if(!executor) { return; }
            if(mission_state != LOADING) { return; }
            // auto &berth = Berths::berths[executor->berth_id];
            if(executor->load == SHIP_CAPACITY) {
                mission_state = SAILING;
                target = no_index;
                Berths::berths[executor->berth_id].occupied = nullptr;
            }
        }
        auto check_emptyload() {
            if(!executor) { return; }
            if(mission_state != LOADING) { return; }
            if(Berths::berths[executor->berth_id].cargo.empty()) {
                mission_state = WAITING;
            }
        }
        auto check_forceback() {
            if(!executor) { return; }
            if(mission_state == SAILING) { return; }
            auto [time, _] = transport(executor->berth_id, no_index);
            if(time == MAX_FRAME - stamp) {
                mission_state = SAILING;
                target = no_index;
                Berths::berths[executor->berth_id].disabled_loading = true;
                Berths::berths[executor->berth_id].occupied = nullptr;
            }
        }
        auto forward() {
            if(!executor) { return; }
            switch(mission_state) {
            case WAITING: {
                DEBUG ::tot_score += executor->load_values;
                executor->load = 0;
                executor->load_values = 0;
            } break;
            case SAILING: {
            } break;
            case LOADING: {
                auto [cnt, load_value] = Berths::berths[executor->berth_id].get_load(SHIP_CAPACITY - executor->load);
                executor->load += cnt;
                executor->load_values += load_value;
            } break;
            case QUEUEING: {
            } break;
            }
            if(!executor->sail_out) {
                next_move = transport(executor->berth_id, target).second;
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

    auto resolve() {
        return std::async(std::launch::async, [this] {
            for(auto &ship: *this) {
                ship.mission.check_emptyload();
                ship.mission.create(&ship);
                ship.mission.check_arrival();
                ship.mission.check_waiting();
                ship.mission.check_loading();
                ship.mission.check_queueing();
                ship.mission.check_overload();
                ship.mission.check_forceback();
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
 * 2. [*已实现] transport, 用于计算从s到t的最优寻路, 此情景下最多中转一次
 * 3. [*已实现] 调度机制, 货物运到的速度<<装载, 所以动态优先调度船去转载货物 (考虑港口间移动)
 * 4. [*已实现] 调度添加边界 (当要结束时, 只考虑目标点位能转载的量)
 *
 * BUGS:
 *  1. [*已解决] Ship每次load最多loading_speed个货物, 但是要看berth够不够
 *  2. [*已修复] Ship在结束前返回的策略不够精确
 * */


#endif//CODECRAFTSDK_SHIP_HPP
