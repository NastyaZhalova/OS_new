#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isWritten;
};

HANDLE freeSlots, filledSlots, fileMutex;
int readIndex = 0;

void initializeFile(const std::string& filename, int count) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    Message empty = { "", false };
    for (int i = 0; i < count; ++i)
        file.write(reinterpret_cast<char*>(&empty), sizeof(Message));
    file.close();
}

bool createSyncObjects(int count) {
    freeSlots = CreateSemaphoreA(NULL, count, count, "FreeSlots");
    filledSlots = CreateSemaphoreA(NULL, 0, count, "FilledSlots");
    fileMutex = CreateMutexA(NULL, FALSE, "FileMutex");
    return freeSlots && filledSlots && fileMutex;
}

void launchSenders(const std::string& filename, int senderCount, std::vector<HANDLE>& readyEvents) {
    for (int i = 0; i < senderCount; ++i) {
        std::string id = std::to_string(i);
        std::string eventName = "SenderReady_" + id;
        HANDLE readyEvent = CreateEventA(NULL, TRUE, FALSE, eventName.c_str());
        readyEvents.push_back(readyEvent);

        std::string cmdStr = "sender.exe " + filename + " " + id;
        std::vector<char> cmd(cmdStr.begin(), cmdStr.end());
        cmd.push_back('\0'); 

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        DWORD flags = CREATE_NEW_CONSOLE; 

        if (CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        else {
            std::cerr << "Failed to start sender " << id << "\n";
        }
    }
}



void readMessage(const std::string& filename, int slotCount) {
    DWORD waitResult = WaitForSingleObject(filledSlots, INFINITE);
    if (waitResult != WAIT_OBJECT_0) return;

    WaitForSingleObject(fileMutex, INFINITE);

    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    file.seekg(readIndex * sizeof(Message));
    Message msg;
    file.read(reinterpret_cast<char*>(&msg), sizeof(Message));

    if (msg.isWritten) {
        std::cout << "Received: " << msg.text << "\n";
        msg.isWritten = false;
        file.seekp(readIndex * sizeof(Message));
        file.write(reinterpret_cast<char*>(&msg), sizeof(Message));
        readIndex = (readIndex + 1) % slotCount;
    }

    file.close();
    ReleaseMutex(fileMutex);
    ReleaseSemaphore(freeSlots, 1, NULL);
}

int main() {
    std::string filename;
    int slotCount, senderCount;

    std::cout << "Enter binary filename: ";
    std::getline(std::cin, filename);
    std::cout << "Enter number of message slots: ";
    std::cin >> slotCount;
    std::cin.ignore();

    initializeFile(filename, slotCount);
    if (!createSyncObjects(slotCount)) {
        std::cerr << "Failed to create sync objects.\n";
        return 1;
    }

    std::cout << "Enter number of Sender processes: ";
    std::cin >> senderCount;
    std::cin.ignore();

    std::vector<HANDLE> readyEvents;
    launchSenders(filename, senderCount, readyEvents);
    WaitForMultipleObjects(senderCount, readyEvents.data(), TRUE, INFINITE);
    std::cout << "All senders are ready.\n";

    while (true) {
        std::string cmd;
        std::cout << "Enter command (read/exit): ";
        std::getline(std::cin, cmd);
        if (cmd == "exit") break;
        if (cmd == "read") readMessage(filename, slotCount);
    }

    std::cout << "Receiver exiting.\n";
    return 0;
}
