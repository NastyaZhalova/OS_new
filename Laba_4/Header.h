#pragma once
#include <windows.h>
#include <string>
#include <vector>

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isWritten;
};

extern HANDLE freeSlots;
extern HANDLE filledSlots;
extern HANDLE fileMutex;
extern int readIndex;

void initializeFile(const std::string& filename, int count);
bool createSyncObjects(int count);
void launchSenders(const std::string& filename, int senderCount, std::vector<HANDLE>& readyEvents);
void readMessage(const std::string& filename, int slotCount);
