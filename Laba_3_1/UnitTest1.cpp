#include "pch.h"
#include "CppUnitTest.h"
#include "Header.h" // путь к marker.h

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
    TEST_CLASS(UnitTest1)
    {
    public:

        TEST_METHOD(TestMarkerThreadExecutionAndClear)
        {
            const int size = 10;
            int array[size] = {};
            CRITICAL_SECTION cs;
            InitializeCriticalSection(&cs);

            MarkerData data;
            data.id = 1;
            data.array = array;
            data.size = size;
            data.cs = &cs;
            data.active = true;
            data.startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            data.resumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            HANDLE thread = CreateThread(NULL, 0, MarkerThread, &data, 0, NULL);

            SetEvent(data.startEvent);

            // Ждём, пока поток не столкнётся с занятым индексом
            WaitForSingleObject(data.doneEvent, 5000);

            // Останавливаем поток
            SetEvent(data.stopEvent);
            WaitForSingleObject(thread, 5000);

            // Проверка: все размеченные индексы должны быть очищены
            for (int idx : data.markedIndices) {
                Assert::AreEqual(0, array[idx], L"Marked index was not cleared");
            }

            // Очистка ресурсов
            CloseHandle(data.startEvent);
            CloseHandle(data.resumeEvent);
            CloseHandle(data.stopEvent);
            CloseHandle(data.doneEvent);
            CloseHandle(thread);
            DeleteCriticalSection(&cs);
        }

        TEST_METHOD(TestMarkerThreadRestartFails)
        {
            const int size = 5;
            int array[size] = {};
            CRITICAL_SECTION cs;
            InitializeCriticalSection(&cs);

            MarkerData data;
            data.id = 2;
            data.array = array;
            data.size = size;
            data.cs = &cs;
            data.active = true;
            data.startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            data.resumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            HANDLE thread = CreateThread(NULL, 0, MarkerThread, &data, 0, NULL);
            SetEvent(data.startEvent);
            WaitForSingleObject(data.doneEvent, 3000);

            SetEvent(data.stopEvent);
            WaitForSingleObject(thread, 3000);

            // Попытка повторного запуска
            SetEvent(data.resumeEvent);
            Sleep(100); // поток уже завершён, ничего не должно произойти

            for (int idx : data.markedIndices) {
                Assert::AreEqual(0, array[idx]);
            }

            CloseHandle(data.startEvent);
            CloseHandle(data.resumeEvent);
            CloseHandle(data.stopEvent);
            CloseHandle(data.doneEvent);
            CloseHandle(thread);
            DeleteCriticalSection(&cs);
        }

        TEST_METHOD(TestUniqueMarkerIdInArray)
        {
            const int size = 10;
            int array[size] = {};
            CRITICAL_SECTION cs;
            InitializeCriticalSection(&cs);

            MarkerData data;
            data.id = 7;
            data.array = array;
            data.size = size;
            data.cs = &cs;
            data.active = true;
            data.startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            data.resumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            data.doneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            HANDLE thread = CreateThread(NULL, 0, MarkerThread, &data, 0, NULL);
            SetEvent(data.startEvent);
            WaitForSingleObject(data.doneEvent, 3000);

            for (int idx : data.markedIndices) {
                Assert::AreEqual(data.id, array[idx]);
            }

            SetEvent(data.stopEvent);
            WaitForSingleObject(thread, 3000);

            CloseHandle(data.startEvent);
            CloseHandle(data.resumeEvent);
            CloseHandle(data.stopEvent);
            CloseHandle(data.doneEvent);
            CloseHandle(thread);
            DeleteCriticalSection(&cs);
        }

    };
}

