#pragma once
#ifndef CODECRAFTSDK_ITEM_HPP
#define CODECRAFTSDK_ITEM_HPP

#include <algorithm>
#include <deque>
#include <istream>

#include "Config.h"
#include "Position.hpp"

struct Item {
    int stamp{};
    long unique_id{};
    Position pos;
    int value{};

    void *occupied{nullptr};
    bool deleted{false};

    static long item_cnt;
    static Item noItem;

    static constexpr int OVERDUE = 1000;
    static constexpr int MAX_ITEM_VALUE = 200;
    [[maybe_unused]] static constexpr int MAX_ITEM_PER_STAMP = 10;

    [[nodiscard]] auto live_time() const {
        return OVERDUE - (::stamp - stamp);
    }
    friend std::istream &operator>>(std::istream &in, Item &it) {
        it.stamp = ::stamp;
        return in >> it.pos >> it.value;
    }
    bool operator<(const Item &o) const {
        return unique_id < o.unique_id;
    }
    bool operator==(const Item &o) const {
        return unique_id == o.unique_id;
    }
    bool operator!=(const Item &o) const {
        return !operator==(o);
    }
};

struct Items : public std::deque<Item> {
    using deque::deque;
    static Items items;

    Items() = default;

    void new_item() {
        emplace_back();
        back().unique_id = ++Item::item_cnt;
    }

    Item &find_by_id(long id) {
        if(id == Item::noItem.unique_id) { return Item::noItem; }
        auto res = std::lower_bound(begin(), end(), id, [](auto &item, auto id) {
            return item.unique_id < id;
        });
        return res->unique_id != id ? Item::noItem : this->operator[](res - begin());
    }
};

/**
 * idea
 *  1. occupied和deleted应该改为一个枚举state (改为指针)
 */

#endif//CODECRAFTSDK_ITEM_HPP
