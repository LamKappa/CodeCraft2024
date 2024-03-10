#pragma once
#ifndef CODECRAFTSDK_MISSION_HPP
#define CODECRAFTSDK_MISSION_HPP

struct Mission {
    bool status;
    static Mission create() {
        return idle;
    }
    static Mission idle;
    [[nodiscard]] auto complete() {
        return status;
    }
};

#endif//CODECRAFTSDK_MISSION_HPP