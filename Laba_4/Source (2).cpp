#include "pch.h"
#include "header.h"
#include <fstream>
#include <iostream>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

HANDLE freeSlots, filledSlots, fileMutex;
int writeIndex = 0;

void sendMessage(const std::string& filename, const std::string& text, int slotCount) {
    DWORD waitResult = WaitForSingleObject(freeSlots, 0);
    if (waitResult != WAIT_OBJECT_0) {
        // suppressed output for testability
    }
    WaitForSingleObject(freeSlots, INFINITE);
    WaitForSingleObject(fileMutex, INFINITE);

    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    file.seekg(writeIndex * sizeof(Message));
    Message msg;
    file.read(reinterpret_cast<char*>(&msg), sizeof(Message));

    if (!msg.isWritten) {
        strncpy_s(msg.text, MAX_MSG_LEN, text.c_str(), _TRUNCATE);
        msg.isWritten = true;
        file.seekp(writeIndex * sizeof(Message));
        file.write(reinterpret_cast<char*>(&msg), sizeof(Message));
        writeIndex = (writeIndex + 1) % slotCount;
    }

    file.close();
    ReleaseMutex(fileMutex);
    ReleaseSemaphore(filledSlots, 1, NULL);
}
