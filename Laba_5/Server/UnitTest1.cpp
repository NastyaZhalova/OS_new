#include "pch.h"
#include "CppUnitTest.h"
#include "Core.h"  

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CoreTests
{
    TEST_CLASS(CoreTests)
    {
    public:

        TEST_METHOD(TestReadStartReturnsRecord)
        {
            CoreState st;
            employee e{ 1, "Ann", 10.5 };
            st.recs.push_back(e);

            ClientRequest req{ OP_READ_START, 1 };
            ServerResponse resp{};
            handleRequestCore(st, req, resp);

            Assert::AreEqual((int)ST_OK, (int)resp.status);
            Assert::AreEqual(1, resp.data.num);
            Assert::AreEqual(std::string("Ann"), std::string(resp.data.name));
            Assert::AreEqual(10.5, resp.data.hours);
        }

        TEST_METHOD(TestReadDoneWithoutReadersFails)
        {
            CoreState st;
            employee e{ 1, "Ann", 10.5 };
            st.recs.push_back(e);

            ClientRequest req{ OP_READ_DONE, 1 };
            ServerResponse resp{};
            handleRequestCore(st, req, resp);

            Assert::AreEqual((int)ST_ERR_PROTOCOL, (int)resp.status);
        }

        TEST_METHOD(TestWriteLockCommitUnlockFlow)
        {
            CoreState st;
            employee e{ 1, "Ann", 10.5 };
            st.recs.push_back(e);

            ServerResponse resp{};

            // Lock
            ClientRequest reqLock{ OP_WRITE_LOCK, 1 };
            handleRequestCore(st, reqLock, resp);
            Assert::AreEqual((int)ST_OK, (int)resp.status);

            // Commit
            employee newE{ 1, "Bob", 20.0 };
            ClientRequest reqCommit{ OP_WRITE_COMMIT, 1, newE };
            handleRequestCore(st, reqCommit, resp);
            Assert::AreEqual((int)ST_OK, (int)resp.status);
            Assert::AreEqual(std::string("Bob"), std::string(st.recs[0].name));
            Assert::AreEqual(20.0, st.recs[0].hours);

            // Unlock
            ClientRequest reqUnlock{ OP_WRITE_UNLOCK, 1 };
            handleRequestCore(st, reqUnlock, resp);
            Assert::AreEqual((int)ST_OK, (int)resp.status);
        }

        TEST_METHOD(TestNotFoundRecord)
        {
            CoreState st; // пусто
            ClientRequest req{ OP_READ_START, 42 };
            ServerResponse resp{};
            handleRequestCore(st, req, resp);

            Assert::AreEqual((int)ST_ERR_NOT_FOUND, (int)resp.status);
        }
    };
}
