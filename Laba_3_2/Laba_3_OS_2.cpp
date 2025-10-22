#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>

using namespace std;

struct MarkerSync {
    mutex mtx;
    condition_variable cv;
    bool resume = false;
    bool terminate = false;
};

struct MarkerData {
    int id;
    int* array;
    int size;
    HANDLE startEvent;
    HANDLE doneEvent;
    MarkerSync* sync;
    vector<int> markedIndices;
    mutex arrayMutex;
};

DWORD WINAPI MarkerThread(LPVOID param) {
    MarkerData* data = (MarkerData*)param;
    srand(data->id);

    WaitForSingleObject(data->startEvent, INFINITE);

    while (true) {
        int index = rand() % data->size;

        {
            lock_guard<mutex> lock(data->arrayMutex);
            if (data->array[index] == 0) {
                this_thread::sleep_for(chrono::milliseconds(5));
                data->array[index] = data->id;
                data->markedIndices.push_back(index);
                this_thread::sleep_for(chrono::milliseconds(5));
                continue;
            }
        }

        cout << "[Marker " << data->id << "] can't mark index " << index
            << ", total marked: " << data->markedIndices.size() << endl;

        SetEvent(data->doneEvent);

        unique_lock<mutex> lock(data->sync->mtx);
        data->sync->cv.wait(lock, [&] { return data->sync->resume || data->sync->terminate; });

        if (data->sync->terminate) {
            lock_guard<mutex> lock(data->arrayMutex);
            for (int idx : data->markedIndices) {
                data->array[idx] = 0;
            }
            return 0;
        }

        data->sync->resume = false;
        ResetEvent(data->doneEvent);
    }
}

void PrintArray(int* arr, int size) {
    cout << "Array: ";
    for (int i = 0; i < size; ++i) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

int main() {
    int size, threadCount;
    cout << "Enter array size: ";
    cin >> size;

    int* array = new int[size]();
    cout << "Enter number of marker threads: ";
    cin >> threadCount;

    vector<HANDLE> threads(threadCount);
    vector<MarkerData> threadData(threadCount);
    vector<MarkerSync*> syncs(threadCount);
    vector<bool> active(threadCount, true);

    for (int i = 0; i < threadCount; ++i) {
        syncs[i] = new MarkerSync();
        threadData[i].id = i + 1;
        threadData[i].array = array;
        threadData[i].size = size;
        threadData[i].sync = syncs[i];
        threadData[i].startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        threadData[i].doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        threads[i] = CreateThread(NULL, 0, MarkerThread, &threadData[i], 0, NULL);
    }

    for (int i = 0; i < threadCount; ++i) {
        SetEvent(threadData[i].startEvent);
    }

    int remaining = threadCount;
    while (remaining > 0) {
        vector<HANDLE> waitEvents;
        for (int i = 0; i < threadCount; ++i) {
            if (active[i]) {
                waitEvents.push_back(threadData[i].doneEvent);
            }
        }

        if (!waitEvents.empty()) {
            WaitForMultipleObjects(waitEvents.size(), waitEvents.data(), TRUE, INFINITE);
        }

        PrintArray(array, size);

        int toStop;
        cout << "Enter marker number to stop: ";
        cin >> toStop;
        toStop--;

        {
            lock_guard<mutex> lock(syncs[toStop]->mtx);
            syncs[toStop]->terminate = true;
        }
        syncs[toStop]->cv.notify_one();

        WaitForSingleObject(threads[toStop], INFINITE);
        active[toStop] = false;
        remaining--;

        PrintArray(array, size);

        for (int i = 0; i < threadCount; ++i) {
            if (active[i]) {
                {
                    lock_guard<mutex> lock(syncs[i]->mtx);
                    syncs[i]->resume = true;
                }
                syncs[i]->cv.notify_one();
            }
        }
    }

    delete[] array;
    for (auto s : syncs) delete s;
    cout << "All marker threads finished." << endl;
    return 0;
}
