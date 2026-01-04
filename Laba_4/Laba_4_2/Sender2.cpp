#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <iostream>
#include <string>
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: sender <shmName> <senderId>\n";
        return 1;
    }

    std::string shmName = argv[1];
    std::string senderId = argv[2];

    shared_memory_object shm(open_only, shmName.c_str(), read_write);
    mapped_region region(shm, read_write);

    void* addr = region.get_address();
    Header* header = static_cast<Header*>(addr);
    Message* buffer = reinterpret_cast<Message*>(static_cast<char*>(addr) + sizeof(Header));

    named_mutex mutex(open_only, "fileMutex");
    named_semaphore freeSlots(open_only, "freeSlots");
    named_semaphore filledSlots(open_only, "filledSlots");

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
            if (text.size() >= MAX_MSG_LEN) text = text.substr(0, MAX_MSG_LEN - 1);

            freeSlots.wait();
            scoped_lock<named_mutex> lock(mutex);

            int pos = header->tail;
            strncpy_s(buffer[pos].text, MAX_MSG_LEN, text.c_str(), MAX_MSG_LEN - 1);
            buffer[pos].isWritten = true;

            header->tail = (header->tail + 1) % header->slotCount;

            filledSlots.post();
            std::cout << "Message sent.\n";
        }
    }

    std::cout << "Sender exiting.\n";
    return 0;
}
