#pragma once
#ifndef CODECRAFTSDK_QUEUE_HPP
#define CODECRAFTSDK_QUEUE_HPP

#include "Config.h"

template<typename T, long MAX_SIZE>
struct Queue {
    T q[MAX_SIZE];
    int front = 0, rear = 0;

    [[nodiscard]] inline bool empty() const {
        return front == rear;
    }

    inline void push(T v) {
        q[rear++] = v;
        if(rear == MAX_SIZE) {
            rear = 0;
        }
    }

    inline T pop() {
        T ret = q[front++];
        if(front == MAX_SIZE) {
            front = 0;
        }
        return ret;
    }

    inline T &peek() {
        return q[front];
    }

    inline T &back() {
        if(rear == 0) {
            return q[MAX_SIZE - 1];
        }
        return q[rear - 1];
    }
};

#endif//CODECRAFTSDK_QUEUE_HPP