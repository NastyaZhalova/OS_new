#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace boost::interprocess;

const int MAX_MSG_LEN = 20;

struct Message {
    char text[MAX_MSG_LEN];
    bool isWritten;
};

struct Header {
    int head;
    int tail;
    int slotCount;
};

int main() {
    std::string shmName;
    int slotCount, senderCount;

    std::cout << "Enter shared memory name: ";
    std::getline(std::cin, shmName);

    std::cout << "Enter number of slots: ";
    std::cin >> slotCount;
    std::cin.ignore();

    shared_memory_object::remove(shmName.c_str());
    shared_memory_object shm(create_only, shmName.c_str(), read_write);
    shm.truncate(sizeof(Header) + slotCount * sizeof(Message));
    mapped_region region(shm, read_write);

    void* addr = region.get_address();
    Header* header = static_cast<Header*>(addr);
    header->head = 0;
    header->tail = 0;
    header->slotCount = slotCount;

    Message* buffer = reinterpret_cast<Message*>(static_cast<char*>(addr) + sizeof(Header));
    for (int i = 0; i < slotCount; ++i) {
        buffer[i].isWritten = false;
        memset(buffer[i].text, 0, MAX_MSG_LEN);
    }

    named_mutex::remove("fileMutex");
    named_semaphore::remove("freeSlots");
    named_semaphore::remove("filledSlots");

    named_mutex mutex(create_only, "fileMutex");
    named_semaphore freeSlots(create_only, "freeSlots", slotCount);
    named_semaphore filledSlots(create_only, "filledSlots", 0);

    std::cout << "Enter number of senders: ";
    std::cin >> senderCount;
    std::cin.ignore();

    for (int i = 0; i < senderCount; ++i) {
        std::string cmdLine = "Sender2.exe " + shmName + " " + std::to_string(i + 1);

        std::vector<char> cmdLineVec(cmdLine.begin(), cmdLine.end());
        cmdLineVec.push_back('\0');

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        if (CreateProcessA(
            nullptr,                      
            cmdLineVec.data(),       
            nullptr, nullptr,             
            FALSE,                        
            CREATE_NEW_CONSOLE,                            
            nullptr, nullptr,         
            &si, &pi))
        {
            std::cout << "Sender " << i + 1 << " started.\n";
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::cerr << "Failed to start sender " << i + 1
                << ". Error: " << GetLastError() << "\n";
        }
    }

    std::cout << "All senders started automatically.\n";

    while (true) {
        std::string cmd;
        std::cout << "Enter command (read/exit): ";
        std::getline(std::cin, cmd);

        if (cmd == "exit") break;
        if (cmd == "read") {
            filledSlots.wait();
            scoped_lock<named_mutex> lock(mutex);

            int pos = header->head;
            if (buffer[pos].isWritten) {
                std::cout << "Received: " << buffer[pos].text << "\n";
                buffer[pos].isWritten = false;
                memset(buffer[pos].text, 0, MAX_MSG_LEN);

                header->head = (header->head + 1) % header->slotCount;
            }

            freeSlots.post();
        }
    }

    std::cout << "Receiver exiting.\n";
    return 0;
}
