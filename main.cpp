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
int idle_cnt = 0;
int tot_score = 0;
#define DEBUG_
#ifdef DEBUG_
#define DEBUG if(true)
u32 RANDOM_SEED = 42;
#else
#define DEBUG if(false)
u32 RANDOM_SEED = random_device{}();
#endif

chrono::high_resolution_clock::time_point start_time;

auto &robots = Robots::robots;
auto &berths = Berths::berths;
auto &ships = Ships::ships;
auto &items = Items::items;
auto &atlas = Atlas::atlas;

std::mt19937 eng(RANDOM_SEED);
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
    }
    cin >> SHIP_CAPACITY;
    cin >> buff;

    atlas.build();
    berths.init();
    DEBUG for(int i = BERTH_NUM - 0; i < BERTH_NUM; i++) {
        berths[berths.srb[i]].disabled = true;
    }

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
        auto &robot = robots[i];

        auto next_move = robot.mission.next_move;
        int move_id = COMMAND.at(next_move);
        if(move_id >= 0) {
            cout << "move " << i << " " << move_id << '\n';
        }

        if(robot.pos + robot.mission.next_move == robot.mission.target[0]) {
            cout << "get " << i << '\n';
        } else if(robot.goods && berths.find_by_pos(robot.mission.target[1]).inside(robot.pos)) {
            cout << "pull " << i << '\n';
        }
        DEBUG if(robot.mission.mission_state == Robot::Mission::MISSION_STATE::IDLING) {
            idle_cnt++;
        }
    }
    for(int i = 0; i < SHIP_NUM; i++) {
        if(ships[i].mission.mission_state == Ship::Mission::MISSION_STATE::SAILING) {
            if(ships[i].sail_out) { continue; }
            auto next_move = ships[i].mission.next_move;
            if(next_move == no_index) {
                cout << "go " << i << '\n';
            } else {
                cout << "ship " << i << " " << (int) next_move << '\n';
            }
            ships[i].sail_out = true;
        }
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
        int last_items = 0, last_value = 0;
        for(auto &berth: berths) {
            cerr << "last_items: " << berth.cargo.size() << '\n';
            last_items += (int) berth.cargo.size();
            while(!berth.cargo.empty()) {
                last_value += berth.cargo.front();
                berth.cargo.pop();
            }
        }
        cerr << "tot_last_items: " << last_items << '\n';
        cerr << "tot_last_values: " << last_value << '\n';
        cerr << "obstacle occurred: " << obstacle_cnt << " times\n";
        cerr << "idle occurred: " << idle_cnt << " times\n";
        cerr << "tot_score: " << tot_score << '\n';
    }

    return 0;
}
