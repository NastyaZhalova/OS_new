#include <windows.h>
#include <iostream>
#include <vector>
#include <limits>
#include <string>

struct MarkerData {
    int id;
    int* array;
    int size;
    HANDLE startEvent;
    HANDLE resumeEvent;
    HANDLE stopEvent; 
    HANDLE doneEvent;
    CRITICAL_SECTION* cs;
    std::vector<int> markedIndices;
    bool active;
};

DWORD WINAPI MarkerThread(LPVOID param) {
    MarkerData* data = static_cast<MarkerData*>(param);
    srand(data->id);

    WaitForSingleObject(data->startEvent, INFINITE);

    while (true) {
        int index = rand() % data->size;

        EnterCriticalSection(data->cs);
        if (data->array[index] == 0) {
            Sleep(5);
            data->array[index] = data->id;
            data->markedIndices.push_back(index);
            Sleep(5);
            LeaveCriticalSection(data->cs);
            continue;
        }
        else {
            std::cout << "[Marker " << data->id << "] can't mark index " << index
                << ", total marked: " << data->markedIndices.size() << std::endl;
            LeaveCriticalSection(data->cs);

            SetEvent(data->doneEvent);

            HANDLE events[2] = { data->resumeEvent, data->stopEvent };
            DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

            if (result == WAIT_OBJECT_0 + 1) {
                EnterCriticalSection(data->cs);
                for (int idx : data->markedIndices) {
                    data->array[idx] = 0;
                }
                LeaveCriticalSection(data->cs);
                return 0;
            }
            else {
                ResetEvent(data->doneEvent);
            }
        }
    }
}

void PrintArray(int* arr, int size) {
    std::cout << "Array: ";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int SafeInput(const std::string& prompt, int minValue, int maxValue) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;

        if (!std::cin.good()) {
            std::cin.clear(); 
            std::cin.ignore(10000, '\n'); 
            std::cout << "Input error. Please try again.\n";
            continue;
        }

        if (value < minValue || value > maxValue) {
            std::cout << "Value out of allowed range [" << minValue << ", " << maxValue << "]. Please try again.\n";
            continue;
        }

        std::cin.ignore(10000, '\n');
        return value;
    }
}


int main() {
    int size = SafeInput("Enter array size: ", 1, 1000000);
    int threadCount = SafeInput("Enter number of marker threads: ", 1, size);

    int* array = new int[size]();
    std::vector<HANDLE> threads(threadCount);
    std::vector<MarkerData> threadData(threadCount);
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    for (int i = 0; i < threadCount; ++i) {
        threadData[i].id = i + 1;
        threadData[i].array = array;
        threadData[i].size = size;
        threadData[i].cs = &cs;
        threadData[i].active = true;
        threadData[i].startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        threadData[i].resumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        threadData[i].stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        threadData[i].doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        threads[i] = CreateThread(NULL, 0, MarkerThread, &threadData[i], 0, NULL);
    }

    for (int i = 0; i < threadCount; ++i) {
        SetEvent(threadData[i].startEvent);
    }

    int active = threadCount;
    while (active > 0) {
        std::vector<HANDLE> waitingEvents;
        for (int i = 0; i < threadCount; ++i) {
            if (threadData[i].active) {
                waitingEvents.push_back(threadData[i].doneEvent);
            }
        }

        if (!waitingEvents.empty()) {
            WaitForMultipleObjects(waitingEvents.size(), waitingEvents.data(), TRUE, INFINITE);
        }

        PrintArray(array, size);

        int toStop = SafeInput("Enter marker number to stop: ", 1, threadCount);
        if (!threadData[toStop - 1].active) {
            std::cout << "Marker " << toStop << " is already stopped. Try another." << std::endl;
            continue;
        }

        SetEvent(threadData[toStop - 1].stopEvent);
        WaitForSingleObject(threads[toStop - 1], INFINITE);
        threadData[toStop - 1].active = false;
        active--;

        PrintArray(array, size);

        for (int i = 0; i < threadCount; ++i) {
            if (threadData[i].active) {
                SetEvent(threadData[i].resumeEvent);
            }
        }
    }

    DeleteCriticalSection(&cs);
    delete[] array;
    std::cout << "All marker threads finished." << std::endl;
    return 0;
}
