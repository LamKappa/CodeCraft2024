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

    bool occupied{};
    std::priority_queue<int> cargo;

    Berth() = default;

    static std::function<bool(index_t)> wanted;

    auto notify(u16 time) {
        if(occupied) { return; }
        if(!wanted) { return; }
        if(wanted(this->id)) { return; }
        occupied = true;
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

#endif//CODECRAFTSDK_BERTH_HPP
