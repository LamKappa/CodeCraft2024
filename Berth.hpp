#pragma once
#ifndef CODECRAFTSDK_BERTH_HPP
#define CODECRAFTSDK_BERTH_HPP

#include <algorithm>
#include <array>
#include <functional>
#include <istream>
#include <numeric>
#include <queue>
#include <tuple>

#include "Atlas.hpp"
#include "Config.h"
#include "Item.hpp"
#include "Position.hpp"
#include "Queue.hpp"

struct Berth {
    index_t id{};
    Position pos;
    int transport_time{};
    int loading_speed{};
    bool disabled = false;

    std::map<u16, u16> values;

    int notified = 0;
    int occupied = 0;
    std::queue<Item> cargo;

    Berth() = default;

    static constexpr int TRANSPORT_TIME = 500;
    static constexpr int MAX_TRANSPORT_TIME = 3000;
    static Berth virtual_berth;

    auto notify(Item &item) {
        if(item.value <= 0) { return; }
        notified++;
    }

    auto sign(Item &item) {
        if(item.value <= 0) { return; }
        item.deleted = true;
        cargo.emplace(item);
        notified--;
    }

    auto get_load(int requirement) {
        int load_item_cnt = 0, load_item_value = 0;
        while(!cargo.empty() && load_item_cnt < requirement && load_item_cnt < loading_speed) {
            load_item_value += cargo.front().value;
            cargo.pop();
            load_item_cnt++;
        }
        return std::make_pair(load_item_cnt, load_item_value);
    }

    [[nodiscard]] auto inside(Position p) {
        return pos.first - 2 <= p.first && p.first <= pos.first + 1 &&
               pos.second - 2 <= p.second && p.second <= pos.second + 1;
    }

    friend auto &operator>>(std::istream &in, Berth &b) {
        in >> b.id >> b.pos >> b.transport_time >> b.loading_speed;
        b.pos = b.pos + Position{2, 2};
        return in;
    }
};

struct Berths : public std::array<Berth, BERTH_NUM> {
    using array::array;
    static Berths berths;
    Berths() = default;

    auto &operator[](index_t i) {
        if(i == no_index) return Berth::virtual_berth;
        return array::operator[](i);
    }

    auto init() {
        async_pool.emplace_back(std::async(std::launch::async, [this]{
            for(auto &berth: *this) {
                Queue<std::pair<Position, u16>, 3 * N> q;
                auto vised = Atlas::atlas.bitmap;
                vised.set(berth.pos);
                q.push({berth.pos, 0});
                while(!q.empty()) {
                    auto [u, dis] = q.pop();
                    berth.values[dis]++;
                    for(auto &move: Move) {
                        auto v = u + move;
                        if(v.outside() || vised.test(v)) { continue; }
                        vised.set(v);
                        q.push({v, dis + 1});
                    }
                }
            }
        }));
    }
};

/**
 * idea:
 * 1. notify机制提醒需要使用此泊位, 目前提醒后是即刻响应并更新occupied的, 并且robot只在拿起物品时提醒
 *  优化
 *      1. 可以做成提醒后回调更新, ship处可以设置若没有空余船进入等待队列等
 *          优点是船的调度逻辑是独立的, 且每次notify都表明有一个新物品要送达
 *          (参数value也是预备此功能添加的)
 * 2. wanted回调用于寻求船, 至于几艘船或者是否响应, 交给回调方处理
 * 3. sign/get_load记录货物运到/上船 (目前好像没有实际作用)
 * BUGS
 *  1. [*已修改] robot.pull如果每帧都执行, 可能中途路过一个berth放进去
 * */

#endif//CODECRAFTSDK_BERTH_HPP
