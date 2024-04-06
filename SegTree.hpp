#pragma once
#ifndef CODECRAFTSDK_SEGTREE_HPP
#define CODECRAFTSDK_SEGTREE_HPP

#include <vector>

template<class Info, class Tag, int LEFT_MARGIN, int RIGHT_MARGIN>
struct SegTree {
    struct Node {
        Info info;
        Tag tag;
        int ch[2] = {-1, -1};
    };
    int root = -1;

    SegTree() { _reserve(root); }
    ~SegTree() {
        if(node.empty()) { return; }
        auto finalize = [&](auto &&finalize, int rt, int l, int r) -> void {
            if(rt == -1 || l >= r) { return; }
            finalize(finalize, node[rt].ch[0], l, (l + r) / 2);
            finalize(finalize, node[rt].ch[1], (l + r) / 2, r);
            _drop(rt);
        };
        finalize(finalize, root, LEFT_MARGIN, RIGHT_MARGIN);
    }

    static std::vector<Node> node;
    static int drop;

    static void _drop(int x) {
        node[x].ch[0] = drop;
        drop = x;
    }
    static int _reserve(int &x) {
        if(x != -1) { return x; }
        int res = node.size();
        if(drop == -1) {
            x = res;
            node.emplace_back();
        } else {
            x = res = drop;
            drop = node[x].ch[0];
            node[x] = Node();
        }
        return res;
    }
    static void _apply(int rt, const Tag &v) {
        if(rt == -1) { return; }
        node[rt].info += v;
        node[rt].tag += v;
    }
    static void _push(int rt) {
        if(node[rt].tag == Tag()) { return; }
        for(auto &ch: node[rt].ch) {
            _apply(ch, node[rt].tag);
        }
        node[rt].tag = Tag();
    }
    static void _pull(int rt) {
        node[rt].info = (node[rt].ch[0] == -1 ? Info() : node[node[rt].ch[0]].info) +
                        (node[rt].ch[1] == -1 ? Info() : node[node[rt].ch[1]].info);
    }

    template<typename Modifier>
    static void _modify(int rt, int l, int r, int L, int R, const Modifier &func) {
        if(R <= l || r <= L) { return; }
        if(L <= l && r <= R) { return (void) (func(rt, l, r)); }
        if(L < (l + r) / 2) { _reserve(node[rt].ch[0]); }
        if(R > (l + r) / 2) { _reserve(node[rt].ch[1]); }
        _push(rt);
        _modify(node[rt].ch[0], l, (l + r) / 2, L, R, func);
        _modify(node[rt].ch[1], (l + r) / 2, r, L, R, func);
        _pull(rt);
    }
    void assign(int L, int R, const Info &v) {
        if(L > R) { return; }
        _modify(root, LEFT_MARGIN, RIGHT_MARGIN, L, R + 1, [&](int rt, int l, int r) {
            node[rt].info = v;
        });
    }
    void assign(int X, const Info &v) { assign(X, X, v); }
    void apply(int L, int R, const Tag &t) {
        if(L > R) { return; }
        _modify(root, LEFT_MARGIN, RIGHT_MARGIN, L, R + 1, [&](int rt, int l, int r) {
            _apply(rt, t);
        });
    }
    void apply(int X, const Tag &t) { apply(X, X, t); }

    explicit operator Info &() {
        return node[root].info;
    }
    Info &operator[](int X) {
        int l = LEFT_MARGIN, r = RIGHT_MARGIN, rt = root;
        while(l + 1 < r) {
            int mid = (l + r) / 2;
            if(X < mid) {
                r = mid, rt = _reserve(node[rt].ch[0]);
            } else {
                l = mid, rt = _reserve(node[rt].ch[1]);
            }
        }
        return node[rt].info;
    }

    template<typename Filter>
    static Info _ask(int rt, int l, int r, int L, int R, const Filter &func) {
        if(R <= l || r <= L || rt == -1) { return Info(); }
        if(L <= l && r <= R) { return func(rt, l, r); }
        _push(rt);
        return _ask(node[rt].ch[0], l, (l + r) / 2, L, R, func) +
               _ask(node[rt].ch[1], (l + r) / 2, r, L, R, func);
    }
    Info ask(int L, int R) {
        if(L > R) { return Info(); }
        return _ask(root, LEFT_MARGIN, RIGHT_MARGIN, L, R + 1, [&](int rt, int l, int r) {
            return node[rt].info;
        });
    }

    static const int npos = -1;
    static int _binary_search(int rt, int l, int r, int L, int R, const Info &info, const Info &target) {
        if(R <= l || r <= L || rt == -1 || info + node[rt].info < target) { return npos; }
        if(l + 1 == r) { return l; }
        _push(rt);
        int chl = node[rt].ch[0], chr = node[rt].ch[1], pos;
        pos = _binary_search(chl, l, (l + r) / 2, L, R, info, target);
        if(pos == npos) {
            pos = _binary_search(chr, (l + r) / 2, r, L, R, info + node[chl].info, target);
        }
        return pos;
    }
    int binary_search(const Info &target) {
        return _binary_search(root, LEFT_MARGIN, RIGHT_MARGIN, LEFT_MARGIN, RIGHT_MARGIN,
                              Info(), target);
    }

    template<typename Predicator>
    static int _merge(int rt1, int rt2, int l, int r, const Predicator &pred) {
        if(rt1 == -1) { return rt2; }
        if(rt2 == -1) { return rt1; }
        if(l + 1 == r) {
            node[rt1].info = pred(node[rt1].info, node[rt2].info);
            return rt1;
        }
        _push(rt1);
        _push(rt2);
        node[rt1].ch[0] = _merge(node[rt1].ch[0], node[rt2].ch[0], l, (l + r) / 2, pred);
        node[rt1].ch[1] = _merge(node[rt1].ch[1], node[rt2].ch[1], (l + r) / 2, r, pred);
        _pull(rt1);
        // _drop(rt2);
        return rt1;
    }
    template<typename Predicator>
    void merge(const SegTree &o, const Predicator &pred = std::plus<Info>()) {
        _merge(root, o.root, LEFT_MARGIN, RIGHT_MARGIN, pred);
    }
};

template<class Info, class Tag, int LEFT_MARGIN, int RIGHT_MARGIN>
std::vector<typename SegTree<Info, Tag, LEFT_MARGIN, RIGHT_MARGIN>::Node>
        SegTree<Info, Tag, LEFT_MARGIN, RIGHT_MARGIN>::node;

template<class Info, class Tag, int LEFT_MARGIN, int RIGHT_MARGIN>
int SegTree<Info, Tag, LEFT_MARGIN, RIGHT_MARGIN>::drop = -1;

#endif