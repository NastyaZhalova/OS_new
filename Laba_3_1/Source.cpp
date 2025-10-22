#include "pch.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <limits>
#include <string>
#include "Header.h"


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
