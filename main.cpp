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
int tot_values = 0;

chrono::high_resolution_clock::time_point start_time;

auto &robots = Robots::robots;
auto &berths = Berths::berths;
auto &ships = Ships::ships;
auto &items = Items::items;
auto &atlas = Atlas::atlas;

std::vector<std::future<void>> async_pool;
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
    // berths.init().wait();
    // robots.init().wait();

    DEBUG {
        for(auto &ft: async_pool) {
            ft.wait();
        }
    }
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(4950) -
                                    (std::chrono::high_resolution_clock::now() - start_time));
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
    while(!items.empty() && (!items.front().occupied || items.front().deleted) && items.front().live_time() < 0) {
        items.pop_front();
    }
    for(int i = 0; i < num; i++) {
        items.emplace_back();
        cin >> items.back();
        DEBUG tot_values += items.back().value;
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
    for(auto &berth: berths) {
        if(berth.disabled_pulling) { continue; }
        auto [time, _] = Ship::transport(berth.id, Berth::virtual_berth.id);
        if(time > MAX_FRAME -
                          (stamp +
                           (berth.notified + berth.cargo.size() - 1) / berth.loading_speed +
                           (berth.occupied ? 0 : Berth::TRANSPORT_TIME)) +
                          1) {
            berth.disabled_pulling = true;
        }
    }

    robots.resolve().wait();

    ships.resolve().wait();
}

void Output() {
    for(int i = 0; i < ROBOT_NUM; i++) {
        auto &robot = robots[i];

        auto next_move = robot.mission.next_move;
        int move_id = COMMAND.at(next_move);
        if(move_id >= 0) {
            cout << "move " << i << " " << move_id << '\n';
        }

        Position dest = robot.pos + robot.mission.next_move;
        if(!robot.mission.targets.empty()) {
            if(dest == Items::items.find_by_id(robot.mission.targets.front().first).pos) {
                cout << "get " << i << '\n';
            } else if(robot.goods && berths[robot.mission.targets.front().second].inside(dest)) {
                cout << "pull " << i << '\n';
            }
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
    DEBUG if(argc > 1 && string(argv[1]) == "DEBUG") {
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
        int left_items = 0, left_value = 0;
        for(auto &berth: berths) {
            // cerr << "left_items: " << berth.cargo.size() << '\n';
            left_items += (int) berth.cargo.size();
            while(!berth.cargo.empty()) {
                left_value += berth.cargo.front().value;
                berth.cargo.pop_front();
            }
        }
        for(auto &ship: ships) {
            tot_score += ship.value;
            ship.value = ship.load = 0;
        }
        // cerr << "tot_left_items: " << left_items << '\n';
        // cerr << "tot_left_values: " << left_value << '\n';
        // cerr << "obstacle occurred: " << obstacle_cnt << " times\n";
        // cerr << "idle occurred: " << idle_cnt << " times\n";
        cerr << "tot_item_values: " << tot_values << '\n';
        cerr << "recall: " << fixed << setprecision(2)
             << 100.f * (float) (tot_score) / (float) (tot_score + left_value) << "% "
             << "(" << tot_score << " / " << tot_score + left_value << ")\n";
    }
    else {
        for(auto &ft: async_pool) {
            ft.wait();
        }
    }

    return 0;
}
