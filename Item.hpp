#pragma once
#ifndef CODECRAFTSDK_ITEM_HPP
#define CODECRAFTSDK_ITEM_HPP

#include <deque>
#include <istream>

#include "Config.h"
#include "Position.hpp"

struct Item {
    int stamp{};
    long unique_id;
    Position pos;
    int value{};

    bool occupied{};

    static long item_cnt;

    static constexpr int MAX_ITEM_PER_STAMP = 10;
    static constexpr int OVERDUE = 1000;

    friend std::istream &operator>>(std::istream &in, Item &it) {
        it.stamp = ::stamp;
        it.unique_id = item_cnt++;
        return in >> it.pos >> it.value;
    }
};

struct Items : public std::deque<Item> {
    using deque::deque;
    static Items items;

    Items() = default;
};

#endif//CODECRAFTSDK_ITEM_HPP
