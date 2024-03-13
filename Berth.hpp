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

    bool disabled = false;
    int notified = 0;
    int occupied = 0;
    std::queue<int> cargo;

    Berth() = default;

    static std::function<bool(index_t)> wanted;
    static constexpr int TRANSPORT_TIME = 500;
    static constexpr int MAX_TRANSPORT_TIME = 3000;
    static Berth virtual_berth;

    [[nodiscard]] auto evaluate() const {
        return 2.f * transport_time + (float) SHIP_CAPACITY / loading_speed;
    }
    auto notify(u16 time) {
        notified++;
        if(occupied > 0) { return; }
        // if(occupied * SHIP_CAPACITY > notified) { return; }
        if(!wanted) { return; }
        wanted(this->id);
    }

    auto sign(int value) {
        cargo.emplace(value);
    }

    auto get_load(int requirement) {
        int load_item_cnt = 0, load_item_value = 0;
        while(!cargo.empty() && load_item_cnt < loading_speed) {
            load_item_value += cargo.front();
            cargo.pop();
            load_item_cnt++;
        }
        return std::make_pair(load_item_cnt, load_item_value);
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

    std::array<index_t, BERTH_NUM> srb{};
    std::map<Flatten_Position, index_t> pos_mapping;

    auto &operator[](index_t i) {
        if(i == no_index) return Berth::virtual_berth;
        return array::operator[](i);
    }

    auto init() {
        std::iota(srb.begin(), srb.end(), 0);
        std::sort(srb.begin(), srb.end(), [](auto i, auto j) {
            return berths[i].evaluate() < berths[j].evaluate();
        });
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
