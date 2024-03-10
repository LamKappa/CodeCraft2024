#include <bits/stdc++.h>
#pragma GCC optimize(3, "inline", "Ofast")

#include "Atlas.hpp"
#include "Berth.hpp"
#include "Config.h"
#include "Item.hpp"
#include "Position.hpp"
#include "Robot.hpp"
#include "Ship.hpp"
using namespace std;

#ifdef NDEBUG
#define DEBUG if(true)
#else
#define DEBUG if(false)
#endif

chrono::high_resolution_clock::time_point start_time;

auto &robots = Robots::robots;
auto &berths = Berths::berths;
auto &ships = Ships::ships;
auto &items = Items::items;
auto &atlas = Atlas::atlas;

int stamp, money;
char buff[256];

void Init() {
    atlas.init();
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            char ch;
            cin>>ch;
            if(static_cast<bool>(BARRIER_SYM.count(ch))) {
                atlas.bitmap.set(Position{i, j});
            }
        }
    }
    for(int i = 0; i < BERTH_NUM; i++) {
        cin >> berths[i];
    }
    cin >> Ship::CAPACITY;
    cin >> buff;

    atlas.build();

    cout << "OK" << endl;
    DEBUG {
        cerr << "build time: "
             << fixed << setprecision(3)
             << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count() / 1000.L
             << " s"
             << endl;
    };
    exit(0);
}

void Input() {
    cin >> stamp >> money;
    int num;
    cin >> num;
    while(!items.empty() && items.front().stamp + Item::MAX_TIME_REMAIN < stamp) {
        items.pop_front();
    }
    for(int i = 0; i < num; i++) {
        cin >> items.new_item();
    }
    for(int i = 0; i < ROBOT_NUM; i++) {
        cin >> robots[i];
    }
    for(int i = 0; i < BOAT_NUM; i++) {
        cin >> ships[i];
    }
    cin >> buff;
}

void Resolve() {
//    auto f1 = robots.resolve();
//    auto f2 = ships.resolve();
}

void Output() {
    for(int i = 0; i < ROBOT_NUM; i++) {
    }
    for(int i = 0; i < BOAT_NUM; i++) {
    }
}

int main() {
    start_time = chrono::high_resolution_clock::now();
    freopen("./test.txt", "r", stdin);
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Init();
    for(int _ = 1; _ <= 15000; _++) {
        Input();
        Resolve();
        Output();
        cout << "OK" << endl;
    }

    return 0;
}
