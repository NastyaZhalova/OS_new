#pragma once
#include "pch.h"
#include <windows.h>
#include <vector>

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

DWORD WINAPI MarkerThread(LPVOID param);
