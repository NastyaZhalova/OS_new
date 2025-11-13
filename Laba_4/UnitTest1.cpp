#include "pch.h"
#include "CppUnitTest.h"
#include <fstream>
#include "header.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ReceiverTests
{
    TEST_CLASS(ReceiverTests)
    {
    public:

        TEST_METHOD(TestInitializeFileCreatesCorrectStructure)
        {
            std::string filename = "test_fifo.bin";
            int slotCount = 4;

            initializeFile(filename, slotCount);

            std::ifstream file(filename, std::ios::binary);
            Assert::IsTrue(file.is_open(), L"File was not created");

            Message msg;
            for (int i = 0; i < slotCount; ++i) {
                file.read(reinterpret_cast<char*>(&msg), sizeof(Message));
                Assert::IsFalse(msg.isWritten, L"Message slot should be empty");
                Assert::AreEqual(std::string(msg.text), std::string(""), L"Message text should be empty");
            }

            file.close();
            std::remove(filename.c_str());
        }

        TEST_METHOD(TestCreateSyncObjectsReturnsValidHandles)
        {
            int slotCount = 3;
            bool success = createSyncObjects(slotCount);

            Assert::IsTrue(success, L"createSyncObjects should return true");
            Assert::IsNotNull(freeSlots, L"freeSlots handle is null");
            Assert::IsNotNull(filledSlots, L"filledSlots handle is null");
            Assert::IsNotNull(fileMutex, L"fileMutex handle is null");

            CloseHandle(freeSlots);
            CloseHandle(filledSlots);
            CloseHandle(fileMutex);
        }

        TEST_METHOD(TestFileSlotsAreZeroed)
        {
            std::string filename = "zero_test.bin";
            int slotCount = 5;

            initializeFile(filename, slotCount);

            std::ifstream file(filename, std::ios::binary);
            Assert::IsTrue(file.is_open(), L"File was not created");

            for (int i = 0; i < slotCount; ++i) {
                Message msg;
                file.read(reinterpret_cast<char*>(&msg), sizeof(Message));

                Assert::IsFalse(msg.isWritten, L"isWritten should be false");

                for (int j = 0; j < MAX_MSG_LEN; ++j) {
                    Assert::AreEqual((char)0, msg.text[j], L"text should be zeroed");
                }
            }

            file.close();
            std::remove(filename.c_str());
        }
    };
}
