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

int idle_cnt = 0;
int tot_score = 25000;
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
            Position p(x, y);
            cin >> atlas.maze[p];
            if(static_cast<bool>(BARRIER_SYM.count(atlas.maze[p]))) {
                atlas.bitmap.set(p);
            } else {
                atlas.bitmap.reset(p);
            }
            switch(atlas.maze[p]) {
            case MAP_SYMBOLS::ROBOT: {
                robot_shop.emplace_back(p);
            } break;
            case MAP_SYMBOLS::SHIP: {
                ship_shop.emplace_back(p);
            } break;
            case MAP_SYMBOLS::COMMIT: {
                commit_point.emplace_back(p);
            } break;
            }
        }
    }
    int B;
    cin >> B;
    for(int i = 0; i < B; i++) {
        berths.emplace_back();
        cin >> berths[i];
    }
    cin >> SHIP_CAPACITY;
    cin >> buff;

    atlas.build();
    async_pool.emplace_back(ships.init());

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
    int num, R, S;
    cin >> num;
    while(!items.empty() && (!items.front().occupied || items.front().deleted) && items.front().live_time() < 0) {
        items.pop_front();
    }
    for(int i = 0; i < num; i++) {
        if(items.empty() || items.back().value != 0) { items.new_item(); }
        cin >> items.back();
        DEBUG tot_values += items.back().value;
    }
    cin >> R;
    ASSERT(R == robots.size());
    for(int i = 0; i < robots.size(); i++) {
        cin >> robots[i];
        assert(robots[i].id == i);
    }
    cin >> S;
    ASSERT(S == ships.size());
    for(int i = 0; i < ships.size(); i++) {
        cin >> ships[i];
    }
    cin >> buff;
}

void Resolve() {
    robots.resolve().wait();

    ships.resolve().wait();
}

void Output() {
    for(int i = 0; i < robots.size(); i++) {
        auto &robot = robots[i];
        auto next_move = robot.mission.next_move;
        Position dest = robot.pos + robot.mission.next_move;

        if(robot.goods && robot.mission.mission_state != Robot::Mission::MISSION_STATE::CARRYING) {
            cout << "pull " << i << '\n';
        }

        int move_id = COMMAND.at(next_move);
        if(move_id >= 0) {
            cout << "move " << i << " " << move_id << '\n';
            if(!robot.mission.targets.empty()) {
                if(dest == Items::items.find_by_id(robot.mission.targets.front().first).pos) {
                    cout << "get " << i << '\n';
                }
            }
        }
        DEBUG if(robot.mission.mission_state == Robot::Mission::MISSION_STATE::IDLING) {
            idle_cnt++;
        }
    }
    for(int i = 0; i < ships.size(); i++) {
        cout << ships[i].output << '\n';
    }
}

int main(int argc, char *argv[]) {
    DEBUG if(argc > 1 && string(argv[1]) == "DEBUG") {
        auto f1 = freopen("../output.txt", "r", stdin);
        auto f2 = freopen("/dev/null", "w", stdout);
    }
    start_time = chrono::high_resolution_clock::now();
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Init();
    int max_robot = 10;
    int robot_avg_alloc = 8 / robot_shop.size();
    {
        // after init
        Input();
        for(auto p: ship_shop) {
            if(!ships.empty()) { break; }
            if(money - SHIP_COST < 0) { break; }
            money -= SHIP_COST;
            ships.new_ship(p);
            cout << "lboat " << (int) p.first << " " << (int) p.second << '\n';
        }
        for(int i = 0; i < robot_avg_alloc; i++) {
            for(int j = 0; j < robot_shop.size(); j++) {
                if(money - ROBOT_COST < 0) { break; }
                money -= ROBOT_COST;
                robots.new_robot(robot_shop[j]);
                cout << "lbot " << (int) robot_shop[j].first << " " << (int) robot_shop[j].second << '\n';
            }
        }
        cout << "OK" << endl;
    }
    for(int _ = 2, j = 0; _ <= MAX_FRAME; _++) {
        Input();
        Resolve();
        Output();
        if(j == robot_shop.size()) j = 0;
        for(; j < robot_shop.size(); j++) {
            if(robots.size() > max_robot) { break; }
            if(money - ROBOT_COST < 0) { break; }
            money -= ROBOT_COST;
            robots.new_robot(robot_shop[j]);
            cout << "lbot " << (int) robot_shop[j].first << " " << (int) robot_shop[j].second << '\n';
        }
        cout << "OK" << endl;
    }

    DEBUG {
        int cost = robots.size() * ROBOT_COST + ships.size() * SHIP_COST;
        int left_items = 0, left_value = 0;
        for(auto &berth: berths) {
            cerr << "left_items: " << berth.cargo.size() << '\n';
            left_items += (int) berth.cargo.size();
            while(!berth.cargo.empty()) {
                left_value += berth.cargo.front().value;
                berth.cargo.pop_front();
            }
        }
        // for(auto &ship: ships) {
        //     tot_score += ship.load_values;
        //     ship.load_values = ship.load = 0;
        // }
        // cerr << "tot_left_items: " << left_items << '\n';
        // cerr << "tot_left_values: " << left_value << '\n';
        // cerr << "idle occurred: " << idle_cnt << " times\n";
        cerr << "tot_item_values: " << tot_values << '\n';
        cerr << "profit: " << fixed << setprecision(2)
             << 100.f * (float) (tot_score + left_value - cost) / (float) (tot_score + left_value) << "% "
             << "(" << tot_score + left_value - cost << " / " << tot_score + left_value << ")\n";
        cerr << "recall: " << fixed << setprecision(2)
             << 100.f * (float) (tot_score) / (float) (tot_score + left_value) << "% "
             << "(" << tot_score << " / " << tot_score + left_value << ")\n";
        cerr << "profit-recall: " << fixed << setprecision(2)
             << 100.f * (float) (tot_score - cost) / (float) (tot_score - cost + left_value) << "% "
             << "(" << tot_score - cost << " / " << tot_score - cost + left_value << ")\n";
    }
    else {
        for(auto &ft: async_pool) {
            ft.wait();
        }
    }

    return 0;
}
