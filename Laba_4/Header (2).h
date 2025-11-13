#pragma once
#include "pch.h"
#include <windows.h>
#include <string>

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isWritten;
};

extern HANDLE freeSlots;
extern HANDLE filledSlots;
extern HANDLE fileMutex;
extern int writeIndex;

void sendMessage(const std::string& filename, const std::string& text, int slotCount);
