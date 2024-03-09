#include <bits/stdc++.h>
#pragma GCC optimize(3)

#include "Atlas.hpp"
#include "Berth.hpp"
#include "Boat.hpp"
#include "Config.h"
#include "Item.hpp"
#include "Position.hpp"
#include "Robot.hpp"
using namespace std;

chrono::high_resolution_clock::time_point start_time;

auto &robots = Robots::robots;
auto &berths = Berths::berths;
auto &boats = Boats::boats;
auto &items = Items::items;
auto &atlas = Atlas::atlas;

int stamp, money;
char buff[256];

void Init() {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            char ch;
            cin >> ch;
            atlas.bitmap_at(i, j) = static_cast<bool>(BARRIER_SYM.count(ch));
        }
    }
    for(int i = 0; i < BERTH_NUM; i++) {
        cin >> berths[i];
    }
    cin >> Boat::CAPACITY;
    cin >> buff;

    atlas.build();

    cout << "OK" << endl;
    cerr << "build time: "
         << chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count()
         << " seconds"
         << endl;
//    exit(0);
}

int Input() {
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
        cin >> boats[i];
    }
    cin >> buff;
    return stamp;
}

int main() {
    start_time = chrono::high_resolution_clock::now();
//    freopen("./test.txt", "r", stdin);
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Init();
    for(int _ = 1; _ <= 15000; _++) {
        Input();
        for(int i = 0; i < ROBOT_NUM; i++) {
            cout << "move " << (i % 10) << " " << (rand() % 4) << '\n';
        }
        //        for(int i = 0; i < BOAT_NUM; i ++)
        //            printf("ship %d %d\n", i % 5, rand() % 10);
        cout << "OK" << endl;
    }

    return 0;
}
