#pragma once
#ifndef CODECRAFTSDK_ITEM_HPP
#define CODECRAFTSDK_ITEM_HPP

#include <istream>
#include <deque>

#include "Config.h"
#include "Position.hpp"

struct Item {
    Position pos;
    int value{};
    int stamp{};

    static constexpr int MAX_TIME_REMAIN = 1000;

    friend std::istream &operator>>(std::istream &in, Item &it) {
        it.stamp = ::stamp;
        return in >> it.pos >> it.value;
    }
};

struct Items : public std::deque<Item> {
    using deque::deque;
    static Items items;

    Items() = default;
    auto &new_item() {
        emplace_back();
        return back();
    }
};

#endif//CODECRAFTSDK_ITEM_HPP
