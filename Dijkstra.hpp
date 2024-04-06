#pragma once
#ifndef CODECRAFTSDK_DIJKSTRA_HPP
#define CODECRAFTSDK_DIJKSTRA_HPP

#include <vector>
#include <queue>

template<typename Edge>
struct DirectedGraph {
    using ValueType = decltype(Edge::w);
    int node_size{}, edge_size{}, tarjan_tot{};
    std::vector<std::vector<Edge> > edge;
    std::vector<bool> vis;
    std::vector<int> dfn, low;

    void resize(int node_size_t) {
        this->node_size = node_size_t;
        edge.resize(node_size_t + 1);
    }

    void add_edge(Edge e) {
        if(e.from == -1 || e.to == -1) {
            return;
        }
        edge_size++;
        edge[e.from].push_back(e);
    }

    std::vector<ValueType> dijkstra(int s) {
        std::vector<ValueType> dis(node_size + 1, -1);
        using Pos = std::pair<ValueType, int>;
        std::priority_queue<Pos, std::vector<Pos>, std::greater<>> q;
        q.emplace(0ll, s);
        while(!q.empty()) {
            auto [d, x] = q.top();
            q.pop();
            if(dis[x] != -1) {
                continue;
            }
            else {
                dis[x] = d;
            }
            for(const Edge &e : edge[x]) {
                if(dis[e.to] != -1) {
                    continue;
                }
                q.emplace(d + e.w, e.to);
            }
        }
        return dis;
    }
};


#endif // CODECRAFTSDK_DIJKSTRA_HPP