#include "pch.h"
#include "CppUnitTest.h"
#include "Header.h" // путь к marker.h

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
    TEST_CLASS(MarkerTests)
    {
    public:

        TEST_METHOD(TestMarkerClearsMarksOnTerminate)
        {
            const int size = 10;
            int array[size] = {};
            MarkerSync sync;
            MarkerData data{ 1, array, size, true, false, &sync };

            std::thread t(MarkerThread, &data);

            // Ждём, пока поток отметит хотя бы одну ячейку
            while (!data.done) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            {
                std::lock_guard<std::mutex> lock(sync.mtx);
                sync.terminate = true;
            }
            sync.cv.notify_one();
            t.join();

            for (int idx : data.markedIndices) {
                Assert::AreEqual(0, array[idx], L"Marked index was not cleared");
            }
        }

        TEST_METHOD(TestMarkerMarksWithCorrectId)
        {
            const int size = 10;
            int array[size] = {};
            MarkerSync sync;
            MarkerData data{ 7, array, size, true, false, &sync };

            std::thread t(MarkerThread, &data);

            while (!data.done) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            {
                std::lock_guard<std::mutex> lock(sync.mtx);
                sync.terminate = true;
            }
            sync.cv.notify_one();
            t.join();

            for (int idx : data.markedIndices) {
                Assert::AreEqual(7, array[idx], L"Marked index does not match marker ID");
            }
        }

        TEST_METHOD(TestMarkerResumeClearsDoneFlag)
        {
            const int size = 10;
            int array[size] = {};
            MarkerSync sync;
            MarkerData data{ 3, array, size, true, false, &sync };

            std::thread t(MarkerThread, &data);

            while (!data.done) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            {
                std::lock_guard<std::mutex> lock(sync.mtx);
                sync.resume = true;
            }
            sync.cv.notify_one();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            Assert::IsFalse(data.done, L"Done flag should be reset after resume");

            {
                std::lock_guard<std::mutex> lock(sync.mtx);
                sync.terminate = true;
            }
            sync.cv.notify_one();
            t.join();
        }

        TEST_METHOD(TestMarkerInitialization)
        {
            int dummyArray[5] = {};
            MarkerSync sync;
            MarkerData data{ 5, dummyArray, 5, false, false, &sync };

            Assert::AreEqual(5, data.id);
            Assert::AreEqual(5, data.size);
            Assert::IsFalse(data.start);
            Assert::IsFalse(data.done);
            Assert::IsNotNull(data.array);
            Assert::IsNotNull(data.sync);
        }
    };
}
