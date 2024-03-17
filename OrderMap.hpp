#pragma once
#ifndef CODECRAFTSDK_ORDERMAP_HPP
#define CODECRAFTSDK_ORDERMAP_HPP

#define _EXT_CODECVT_SPECIALIZATIONS_H 1
#define _EXT_ENC_FILEBUF_H 1
#include <bits/extc++.h>

template<typename K_T,
         typename V_T = __gnu_pbds::null_type,
         typename Cmp = std::less<K_T>
         >
using OrderSet = __gnu_pbds::tree<
        K_T,                                                // key_type
        V_T,                                                // value_type
        Cmp,                                                // comparator
        __gnu_pbds::rb_tree_tag,                            // tag
        __gnu_pbds::tree_order_statistics_node_update       // policy
        >;
template<typename K_T,
         typename V_T = __gnu_pbds::null_type,
         typename Cmp = std::less<K_T>
         >
using OrderMap = OrderSet<K_T, V_T, Cmp>;
/*
tag:
rb_tree_tag                                 红黑树
splay_tree_tag                              Slpay
ov_tree_tag                                 向量树

Itr ::point_iterator

std::pair<point_iterator, bool> insert(T)   插入
bool erase(T/Itr)                           删除元素/迭代器
int order_of_key(T)                         返回排名
Itr find_by_order(T)                        排名对应元素
Itr lower_bound(T)/upper_bound(T)           二分查找
void join(order_set)                        合并
void split(T,order_set)                     保留<=,其余分离覆盖到order_set中
bool empty()                                判空
size_t size()                               大小
Itr begin()/end()                           首尾迭代器
*/
#endif