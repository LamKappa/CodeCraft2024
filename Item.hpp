#pragma once
#ifndef CODECRAFTSDK_ITEM_HPP
#define CODECRAFTSDK_ITEM_HPP

#include <deque>
#include <istream>
#include <algorithm>

#include "Config.h"
#include "Position.hpp"

struct Item {
    int stamp{};
    long unique_id;
    Position pos;
    int value{};

    bool occupied{};

    static long item_cnt;
    static Item noItem;

    static constexpr int MAX_ITEM_PER_STAMP = 10;
    static constexpr int OVERDUE = 1000;
    static constexpr int MAX_ITEM_VALUE = 200;

    [[nodiscard]] auto live_time()const{
        return OVERDUE - (::stamp - stamp);
    }
    friend std::istream &operator>>(std::istream &in, Item &it) {
        it.stamp = ::stamp;
        it.unique_id = item_cnt++;
        return in >> it.pos >> it.value;
    }
    bool operator<(const Item&o)const{
        return unique_id < o.unique_id;
    }
};

struct Items : public std::deque<Item> {
    using deque::deque;
    static Items items;

    Items() = default;

    static Item&find_by_id(long id){
        auto res = std::lower_bound(items.begin(), items.end(), id, [](auto&item, auto id){
            return item.unique_id < id;
        });
        return res == items.end() ? Item::noItem : *res;
    }
};

#endif//CODECRAFTSDK_ITEM_HPP
