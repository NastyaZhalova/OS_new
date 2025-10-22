#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>

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

int SafeInput(const std::string& prompt, int minValue, int maxValue) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;

        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Input error. Please enter a number.\n";
            continue;
        }

        if (value < minValue || value > maxValue) {
            std::cout << "Value out of allowed range [" << minValue << ", " << maxValue << "]. Try again.\n";
            continue;
        }

        std::cin.ignore(10000, '\n');
        return value;
    }
}

void MarkerThread(MarkerData* data) {
    srand(data->id);

    // Wait for start signal
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

        std::cout << "[Marker " << data->id << "] can't mark index " << index
            << ", total marked: " << data->markedIndices.size() << std::endl;

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

void PrintArray(int* arr, int size) {
    std::cout << "Array: ";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    int size = SafeInput("Enter array size: ", 1, 1000000);
    int threadCount = SafeInput("Enter number of marker threads: ", 1, size);

    int* array = new int[size]();
    std::vector<std::thread> threads(threadCount);
    std::vector<MarkerData> threadData(threadCount);
    std::vector<MarkerSync*> syncs(threadCount);
    std::vector<bool> active(threadCount, true);

    for (int i = 0; i < threadCount; ++i) {
        syncs[i] = new MarkerSync();
        threadData[i].id = i + 1;
        threadData[i].array = array;
        threadData[i].size = size;
        threadData[i].sync = syncs[i];

        threads[i] = std::thread(MarkerThread, &threadData[i]);
    }

    for (int i = 0; i < threadCount; ++i) {
        threadData[i].start = true;
    }

    int remaining = threadCount;
    while (remaining > 0) {
        // Wait for all active threads to reach done
        for (int i = 0; i < threadCount; ++i) {
            while (active[i] && !threadData[i].done) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        PrintArray(array, size);

        int toStop = SafeInput("Enter marker number to stop: ", 1, threadCount);
        toStop--;

        if (!active[toStop]) {
            std::cout << "Marker " << (toStop + 1) << " is already stopped. Try another.\n";
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(syncs[toStop]->mtx);
            syncs[toStop]->terminate = true;
        }
        syncs[toStop]->cv.notify_one();

        threads[toStop].join();
        active[toStop] = false;
        remaining--;

        PrintArray(array, size);

        for (int i = 0; i < threadCount; ++i) {
            if (active[i]) {
                {
                    std::lock_guard<std::mutex> lock(syncs[i]->mtx);
                    syncs[i]->resume = true;
                }
                syncs[i]->cv.notify_one();
            }
        }
    }

    delete[] array;
    for (auto s : syncs) delete s;
    std::cout << "All marker threads finished." << std::endl;
    return 0;
}
