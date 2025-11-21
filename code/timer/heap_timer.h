#pragma once

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../Log/Log.h"

// 定义定时器超时后回调函数类型：无参数且无返回值的函数
typedef std::function<void()> TimeoutCallBack;

// 高精度时钟，用于精确记录时间点
typedef std::chrono::high_resolution_clock Clock;

// 毫秒单位时间表示
typedef std::chrono::milliseconds MS;

// 定义时间戳类型
typedef Clock::time_point TimeStamp; // 用于记录定时器的过期时间点

// 定时器节点结构体，包含定时器的ID、过期时间和回调函数
struct TimerNode {
    int id;                // 定时器ID
    TimeStamp expires;     // 定时器的过期时间
    TimeoutCallBack cb;    // 定时器超时后的回调函数

    // 重载小于运算符，用于堆排序，堆顶元素是最早过期的定时器
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

// 定时器管理类：基于堆实现的定时器管理器
class HeapTimer {
public:
    // 构造函数，初始化堆并预留64个元素空间
    HeapTimer() {
        heap_.reserve(64);
    }

    // 析构函数，清理定时器
    ~HeapTimer() {
        Clear();
    }

    // 调整定时器的过期时间
    void Adjust(int id, int newExpires);

    // 添加一个新的定时器
    void Add(int id, int timeOut, const TimeoutCallBack& cb);

    // 删除指定id结点，并触发回调函数 
    void DoWork(int id);

    // 清空所有定时器
    void Clear();

    // 清除超时结点
    void Tick();

    // 移除堆顶元素（即最早过期的定时器）
    void Pop();

    // 获取下一个定时器的超时时间
    int GetNextTick();

private:
    // 删除堆中指定位置的定时器
    void Del_(size_t i);

    // 将堆中元素向上调整，保持堆的有序性
    void SiftUp_(size_t i);

    // 将堆中元素向下调整，保持堆的有序性
    bool SiftDown_(size_t index, size_t n);

    // 交换堆中两个元素的位置
    void SwapNode_(size_t i, size_t j);

    // 定时器堆，存储所有定时器节点
    std::vector<TimerNode> heap_;

    // 定时器ID与堆中位置的映射，快速查找
    std::unordered_map<int, size_t> ref_;
};

// 解释：
/*
 * 1. HeapTimer 使用堆（`heap_`）和哈希表（`ref_`）高效管理定时器。
 * 2. 每个定时器由 `TimerNode` 表示，存储定时器的 ID、过期时间和回调函数。
 * 3. 定时器按照过期时间排序，堆顶始终是最早过期的定时器，使用堆的性质高效地添加、删除、调整定时器。
 * 4. `Add()` 方法添加新的定时器，`Adjust()` 方法调整现有定时器，`DoWork()` 执行指定定时器的回调，`Tick()` 方法检查并执行所有已过期的定时器。
 * 5. 使用 `Pop()` 移除堆顶定时器，`GetNextTick()` 获取下一个定时器的超时时间。
 * 6. `Clear()` 方法用于清空所有定时器。
 */
