#include "pch.h"
#include "CppUnitTest.h"
#include <fstream>
#include "header.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SenderTests
{
    TEST_CLASS(SenderTests)
    {
    public:

        TEST_METHOD(TestSendMessageWritesCorrectly)
        {
            std::string filename = "test_sender.bin";
            int slotCount = 3;
            writeIndex = 0;

            std::ofstream file(filename, std::ios::binary | std::ios::trunc);
            Message empty = { "", false };
            for (int i = 0; i < slotCount; ++i)
                file.write(reinterpret_cast<char*>(&empty), sizeof(Message));
            file.close();

            freeSlots = CreateSemaphoreA(NULL, slotCount, slotCount, NULL);
            filledSlots = CreateSemaphoreA(NULL, 0, slotCount, NULL);
            fileMutex = CreateMutexA(NULL, FALSE, NULL);

            sendMessage(filename, "TestMessage", slotCount);

            std::ifstream checkFile(filename, std::ios::binary);
            Message msg;
            checkFile.read(reinterpret_cast<char*>(&msg), sizeof(Message));
            checkFile.close();

            Assert::IsTrue(msg.isWritten);
            Assert::AreEqual(std::string(msg.text), std::string("TestMessage"));

            CloseHandle(freeSlots);
            CloseHandle(filledSlots);
            CloseHandle(fileMutex);
            std::remove(filename.c_str());
        }
    };
}
