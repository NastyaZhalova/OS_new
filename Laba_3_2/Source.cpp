#include "pch.h"
#include "Header.h"
#include <thread>
#include <chrono>
#include <iostream>

void MarkerThread(MarkerData* data) {
    srand(data->id);
    while (!data->start) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    while (true) {
        int index = rand() % data->size;

        {
            std::lock_guard<std::mutex> lock(data->arrayMutex);
            if (data->array[index] == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                data->array[index] = data->id;
                data->markedIndices.push_back(index);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
        }

        data->done = true;

        std::unique_lock<std::mutex> lock(data->sync->mtx);
        data->sync->cv.wait(lock, [&] { return data->sync->resume || data->sync->terminate; });

        if (data->sync->terminate) {
            std::lock_guard<std::mutex> lock(data->arrayMutex);
            for (int idx : data->markedIndices) {
                data->array[idx] = 0;
            }
            return;
        }

        data->sync->resume = false;
        data->done = false;
    }
}
