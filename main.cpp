#include <bits/stdc++.h>
#pragma GCC optimize(3, "Ofast", "inline")

#include "Atlas.hpp"
#include "Berth.hpp"
#include "Config.h"
#include "Item.hpp"
#include "Position.hpp"
#include "Robot.hpp"
#include "Ship.hpp"
using namespace std;

int obstacle_cnt = 0;
#define DEBUG_
#ifdef DEBUG_
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

std::mt19937 eng(random_device{}());
int SHIP_CAPACITY;
int stamp, money;
char buff[256];

void Init() {
    atlas.init();
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < N; y++) {
            char ch;
            cin >> ch;
            if(static_cast<bool>(BARRIER_SYM.count(ch))) {
                atlas.bitmap.set(Position{x, y});
            } else {
                atlas.bitmap.reset(Position{x, y});
            }
        }
    }
    for(int i = 0; i < BERTH_NUM; i++) {
        cin >> berths[i];
        // if(eng() % BERTH_NUM >= SHIP_NUM){
        //     berths[i].disabled = true;
        // }
    }
    cin >> SHIP_CAPACITY;
    cin >> buff;

    atlas.build();
    Berth::wanted = Ships::wanted;
    berths.init();

    cout << "OK" << endl;
    DEBUG {
        cerr << "build time: "
             << fixed << setprecision(3)
             << chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count() / 1000.L
             << " s"
             << endl;
        // exit(0);
    }
}

void Input() {
    cin >> stamp >> money;
    int num;
    cin >> num;
    while(!items.empty() && items.front().stamp + Item::OVERDUE < stamp) {
        items.pop_back();
    }
    for(int i = 0; i < num; i++) {
        items.emplace_back();
        cin >> items.back();
    }
    for(int i = 0; i < ROBOT_NUM; i++) {
        cin >> robots[i];
        DEBUG if(robots[i].status == 0) {
            obstacle_cnt++;
        }
    }
    for(int i = 0; i < SHIP_NUM; i++) {
        cin >> ships[i];
    }
    cin >> buff;
}

void Resolve() {
    auto f1 = robots.resolve();
    f1.wait();

    auto f2 = ships.resolve();
    f2.wait();
}

void Output() {
    for(int i = 0; i < ROBOT_NUM; i++) {
        auto next_move = robots[i].mission.next_move;
        int move_id = COMMAND.at(next_move);
        if(move_id >= 0) {
            cout << "move " << i << " " << move_id << '\n';
        }

        if(!robots[i].goods) {
            cout << "get " << i << '\n';
        } else {
            cout << "pull " << i << '\n';
        }
    }
    for(int i = 0; i < SHIP_NUM; i++) {
        if(ships[i].berth_id != no_index &&
           berths[ships[i].berth_id].transport_time + 1 >= MAX_FRAME - stamp) {
            cout << "go " << i << '\n';
            continue;
        }
        if(ships[i].mission.mission_state == Ship::Mission::MISSION_STATE::SAILING) {
            if(ships[i].sail_out) { continue; }
            auto next_move = Ship::transport(ships[i].berth_id, ships[i].mission.target);
            // cerr << "transport " << i << " " << (int) ships[i].berth_id << " to " << (int) ships[i].mission.target << endl;
            if(next_move == no_index) {
                cout << "go " << i << '\n';
                // cerr << "go " << i << endl;
            } else {
                cout << "ship " << i << " " << (int) next_move << '\n';
                // cerr << "ship " << i << " " << (int) next_move << endl;
            }
            ships[i].sail_out = true;
        }
        // if(stamp == 1) cout << "ship " << i << " " << (int)(i) << '\n';
    }
}

int main(int argc, char *argv[]) {
    if(argc > 1 && string(argv[1]) == "DEBUG") {
        freopen("../output.txt", "r", stdin);
        freopen("/dev/null", "w", stdout);
    }
    start_time = chrono::high_resolution_clock::now();
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Init();
    for(int _ = 1; _ <= MAX_FRAME; _++) {
        Input();
        Resolve();
        Output();
        cout << "OK" << endl;
    }
    DEBUG {
        cerr << "obstacle occurred: " << obstacle_cnt << " times\n";
    }

    return 0;
}
