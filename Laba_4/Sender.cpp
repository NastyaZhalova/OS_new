#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isWritten;
};

HANDLE freeSlots, filledSlots, fileMutex;
int writeIndex = 0;

void sendMessage(const std::string& filename, const std::string& text, int slotCount) {
    DWORD waitResult = WaitForSingleObject(freeSlots, 0);
    if (waitResult != WAIT_OBJECT_0) {
        std::cout << "Waiting for free slot\n";
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
        std::cout << "Message sent.\n";
    }

    file.close();
    ReleaseMutex(fileMutex);
    ReleaseSemaphore(filledSlots, 1, NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: sender.exe <filename> <sender_id>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::string senderId = argv[2];

    freeSlots = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "FreeSlots");
    filledSlots = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "FilledSlots");
    fileMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "FileMutex");

    if (!freeSlots || !filledSlots || !fileMutex) {
        std::cerr << "Failed to open sync objects.\n";
        return 1;
    }

    std::string eventName = "SenderReady_" + senderId;
    HANDLE readyEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
    if (readyEvent) SetEvent(readyEvent);

    std::cout << "Sender " << senderId << " ready.\n";

    while (true) {
        std::string cmd;
        std::cout << "Enter command (send/exit): ";
        std::getline(std::cin, cmd);
        if (cmd == "exit") break;
        if (cmd == "send") {
            std::string text;
            std::cout << "Enter message (<20 chars): ";
            std::getline(std::cin, text);
            if (text.length() >= MAX_MSG_LEN) text = text.substr(0, MAX_MSG_LEN - 1);
            sendMessage(filename, text, MAX_MSG_LEN);
        }
    }

    std::cout << "Sender exiting.\n";
    return 0;
}
