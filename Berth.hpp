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

#include "Config.h"
#include "Position.hpp"

struct Berth {
    index_t id{};
    Position pos;
    int transport_time{};
    int loading_speed{};

    int notified = 0;
    int occupied = 0;
    bool disabled = false;
    std::priority_queue<int> cargo;

    Berth() = default;

    static std::function<bool(index_t)> wanted;

    auto notify(u16 time) {
        notified++;
        if(occupied > 0) { return; }
        if(!wanted) { return; }
        wanted(this->id);
    }

    auto sign(int value) {
        cargo.emplace(value);
    }

    int get_load() {
        int load_item_value = 0;
        if(cargo.empty()) { return load_item_value; }
        load_item_value = cargo.top();
        cargo.pop();
        return load_item_value;
    }

    friend auto &operator>>(std::istream &in, Berth &b) {
        in >> b.id >> b.pos >> b.transport_time >> b.loading_speed;
        b.pos = b.pos + Position{1, 1};
        return in;
    }
};

struct Berths : public std::array<Berth, BERTH_NUM> {
    using array::array;
    static Berths berths;
    Berths() = default;

    std::map<Flatten_Position, index_t> pos_mapping;

    auto init() {
        for(int i = 0; i < BERTH_NUM; i++) {
            pos_mapping[berths[i].pos] = i;
        }
    }

    auto &find_by_pos(Flatten_Position pos) {
        return this->operator[](pos_mapping[pos]);
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
 * */

#endif//CODECRAFTSDK_BERTH_HPP
