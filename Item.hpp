#pragma once
#ifndef CODECRAFTSDK_ITEM_HPP
#define CODECRAFTSDK_ITEM_HPP

#include <istream>

#include "Config.h"
#include "Position.hpp"
#include "Queue.hpp"

struct Item {
    Position pos;
    int value{};
    int stamp{};

    static constexpr int MAX_ITEM_PER_STAMP = 10;
    static constexpr int OVERDUE = 1000;

    friend std::istream &operator>>(std::istream &in, Item &it) {
        it.stamp = ::stamp;
        return in >> it.pos >> it.value;
    }
};

struct Items : public Queue<Item, Item::MAX_ITEM_PER_STAMP * Item::OVERDUE> {
    static Items items;

    Items() = default;
    auto &new_item() {
        push({});
        return back();
    }
};

#endif//CODECRAFTSDK_ITEM_HPP
