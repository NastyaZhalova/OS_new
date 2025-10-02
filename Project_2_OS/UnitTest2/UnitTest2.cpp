#include "pch.h"
#include "CppUnitTest.h"
#include "Header.h" 
#include <sstream>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
    TEST_CLASS(ArrayThreadTests)
    {
    public:

        TEST_METHOD(Test_ValidInput_ThreadsWork)
        {
            arr = { 10, 20, 30, 40, 50 };

            std::thread t1(MinMaxThread);
            std::thread t2(AverageThread);

            t1.join();
            t2.join();

            Assert::AreEqual(10.0, minVal);
            Assert::AreEqual(50.0, maxVal);
            Assert::AreEqual(30.0, averageVal);
        }

        TEST_METHOD(Test_ReplacementLogic)
        {
            arr = { 10, 20, 30, 40, 50 };

            std::thread t1(MinMaxThread);
            std::thread t2(AverageThread);

            t1.join();
            t2.join();

            for (double& x : arr) {
                if (x == minVal || x == maxVal) {
                    x = averageVal;
                }
            }

            Assert::AreEqual(averageVal, arr[0]);
            Assert::AreEqual(averageVal, arr[4]);
            Assert::AreEqual(20.0, arr[1]);
            Assert::AreEqual(30.0, arr[2]);
            Assert::AreEqual(40.0, arr[3]);
        }

        TEST_METHOD(Test_InvalidSizeInput)
        {
            std::istringstream input("-3\n10 20 30");
            bool result = ReadArrayFromStream(input);
            Assert::IsFalse(result);
        }

        TEST_METHOD(Test_InvalidElementInput)
        {
            std::istringstream input("3\n10 abc 30");
            bool result = ReadArrayFromStream(input);
            Assert::IsFalse(result);
        }

        TEST_METHOD(Test_EmptyInput)
        {
            std::istringstream input("");
            bool result = ReadArrayFromStream(input);
            Assert::IsFalse(result);
        }

        TEST_METHOD(Test_AllElementsEqual)
        {
            arr = { 10, 10, 10, 10 };

            std::thread t1(MinMaxThread);
            std::thread t2(AverageThread);

            t1.join();
            t2.join();

            Assert::AreEqual(10.0, minVal);
            Assert::AreEqual(10.0, maxVal);
            Assert::AreEqual(10.0, averageVal);
        }
    };
}
