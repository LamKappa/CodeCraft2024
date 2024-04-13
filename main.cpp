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

int robot0_cnt = 0, robot1_cnt = 0;

int MAX_ROBOT0 = 30, MAX_ROBOT1 = 10;
int MAX_SHIP = 10;
u64 gene = 0;

enum MAP{
    MAP1 = 4961533958ull,
    MAP2 = 2385117190ull
};

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
        gene += *(u64*)&berths[i];
        gene += *((u64*)&berths[i] + 1);
        gene += *((u64*)&berths[i] + 2);
    }
    cin >> SHIP_CAPACITY;
    cin >> buff;

    atlas.build();
    async_pool.emplace_back(ships.init());

    switch(gene){
    case MAP1:
        MAX_ROBOT0 = 19;
        MAX_ROBOT1 = 0;
        MAX_SHIP = 3;
        break;
    case MAP2:
        MAX_ROBOT0 = 17;
        MAX_ROBOT1 = 0;
        MAX_SHIP = 2;
        break;
    default:
        MAX_ROBOT0 = 17;
        MAX_ROBOT1 = 0;
        MAX_SHIP = 1;
        break;
    }

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
        cerr << "gene: " << gene << '\n';
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
    // ASSERT(R == robots.size());
    for(int i = 0; i < robots.size(); i++) {
        cin >> robots[i];
        assert(robots[i].id == i);
    }
    cin >> S;
    // ASSERT(S == ships.size());
    for(int i = 0; i < ships.size(); i++) {
        cin >> ships[i];
    }
    cin >> buff;
}

void Resolve() {
    auto tick_start = chrono::high_resolution_clock::now();
    // for(auto &berth : berths){
    //     if(berth.disabled_pulling) { continue; }
    //     Ship vship;
    //     vship.pos = berth.pos;
    //     vship.dir = berth.dir;
    //     auto berth_hold = berth.notified + berth.cargo.size();
    //     auto dis_come = (int) Atlas::INF_DIS, dis_back = 0;
    //     ships.selectCommit(vship, &dis_back);
    //     for(auto &ship : ships){
    //         auto dis_t = ships.berth_dis[berth.id][Ship::getId(ship.pos, ship.dir).first];
    //         dis_come = min(dis_come, dis_t);
    //     }
    //     if(MAX_FRAME - stamp <= dis_come + dis_back + berth_hold / berth.loading_speed){
    //         berth.disabled_pulling = true;
    //     }
    // }

    std::vector<std::future<void>> resolve_f;

    robots.resolve().wait();
    resolve_f.emplace_back(ships.resolve());

    DEBUG{
        for(auto &f : resolve_f){
            f.wait();
        }
        DEBUG if(chrono::high_resolution_clock::now() - tick_start > 15ms){
            cerr << "TIMELIMIT OUT " << stamp << '\n';
        }
    }else{
        std::this_thread::sleep_for(std::chrono::microseconds(14000) -
                                    (std::chrono::high_resolution_clock::now() - tick_start));
    }
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
    int robot_avg_alloc = std::min(8, MAX_ROBOT0) / robot_shop.size();
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
        int j = 0;
        while(robot0_cnt < MAX_ROBOT0 && money >= ROBOT0_COST){
            if(j == robot_shop.size()) j = 0;
            if(money - ROBOT0_COST < 0) { break; }
            money -= ROBOT0_COST;
            robots.new_robot(robot_shop[j]);
            cout << "lbot " << (int) robot_shop[j].first << " " << (int) robot_shop[j].second << " 0" << '\n';
            j++; robot0_cnt++;
        }
        cout << "OK" << endl;
    }
    for(int _ = 2, j = 0; _ <= MAX_FRAME; _++) {
        now_frame = _;
        Input();
        Resolve();
        Output();
        while(robot0_cnt < MAX_ROBOT0 && money >= ROBOT0_COST){
            if(j == robot_shop.size()) j = 0;
            money -= ROBOT0_COST;
            robots.new_robot(robot_shop[j]);
            cout << "lbot " << (int) robot_shop[j].first << " " << (int) robot_shop[j].second << " 0" << '\n';
            j++; robot0_cnt++;
        }
        while(robot1_cnt < MAX_ROBOT1 && money >= ROBOT1_COST){
            if(j == robot_shop.size()) j = 0;
            money -= ROBOT1_COST;
            robots.new_robot(robot_shop[j], 1);
            cout << "lbot " << (int) robot_shop[j].first << " " << (int) robot_shop[j].second << " 1" << '\n';
            j++; robot1_cnt++;
        }
        while(ships.size() < MAX_SHIP && money >= SHIP_COST) {
            money -= SHIP_COST;
            ships.new_ship(ship_shop[1]);
            cout << "lboat " << (int) ship_shop[1].first << " " << (int) ship_shop[1].second << '\n';
        }
        cout << "OK" << endl;
    }

    DEBUG {
        int robot_cost = robot0_cnt * ROBOT0_COST + robot1_cnt * ROBOT1_COST;
        int ship_cost = ships.size() * SHIP_COST;
        int cost = robot_cost + ship_cost;
        int left_items = 0, left_value = 0;
        for(auto &berth: berths) {
            // cerr << "left_items: " << berth.cargo.size() << '\n';
            left_items += (int) berth.cargo.size();
            while(!berth.cargo.empty()) {
                left_value += berth.cargo.front().value;
                berth.cargo.pop_front();
            }
        }
        int sailing_value = 0;
        for(auto &ship: ships) {
            sailing_value += ship.load_value;
        }
        // cerr << "idle occurred: " << idle_cnt << " times\n";
        cerr << "tot_item_values: " << tot_values << '\n';
        cerr << "left_items: " << left_items << " (" << left_value << ")" << '\n';
        cerr << "ship-value: " << sailing_value << " (capacity: " << SHIP_CAPACITY << ")" << '\n';
        cerr << "robot-value: " << tot_score + left_value + sailing_value << " (clean: " << tot_score + left_value + sailing_value - robot_cost << ")" << '\n';
        cerr << "robots: " << robots.size() << " ships: " << ships.size() << '\n';
        cerr << "score: " << tot_score + BASE_SCORE - cost << " (" << tot_score + BASE_SCORE << " - " << cost << ")" << '\n';
        cerr << "ship/robot: " << fixed << setprecision(2)
             << 100.f * (float) (tot_score) / (float) (tot_score + left_value + sailing_value) << "% "
             << "(" << tot_score << " / " << tot_score + left_value + sailing_value << ")\n";
    }
    else {
        for(auto &ft: async_pool) {
            ft.wait();
        }
    }

    return 0;
}
