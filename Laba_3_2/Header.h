#pragma once
#include "pch.h"
#include <vector>
#include <mutex>
#include <condition_variable>

struct MarkerSync {
    std::mutex mtx;
    std::condition_variable cv;
    bool resume = false;
    bool terminate = false;
};

struct MarkerData {
    int id;
    int* array;
    int size;
    bool start = false;
    bool done = false;
    MarkerSync* sync;
    std::vector<int> markedIndices;
    std::mutex arrayMutex;
};

void MarkerThread(MarkerData* data);
