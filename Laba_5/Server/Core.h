#pragma once
#include "pch.h"
#include <vector>
#include <string>

struct employee {
    int    num;
    char   name[10];
    double hours;
};

enum OpCode { OP_READ_START, OP_READ_DONE, OP_WRITE_LOCK, OP_WRITE_COMMIT, OP_WRITE_UNLOCK, OP_EXIT };
enum Status { ST_OK, ST_ERR_NOT_FOUND, ST_ERR_BUSY, ST_ERR_PROTOCOL };

struct ClientRequest {
    OpCode    op;
    int       num;
    employee  data;
    unsigned long pid;
};

struct ServerResponse {
    Status    status;
    employee  data;
};

struct CoreState {
    std::vector<employee> recs;
    std::wstring fileName;

    struct Lock {
        int readers = 0;
        bool writerLocked = false;
    };
    std::vector<std::pair<int, Lock>> locks;
};

int findIndexById(const CoreState& st, int key);
CoreState::Lock* getLock(CoreState& st, int key);
bool writeRecordToFile(const CoreState& st, int idx);
void handleRequestCore(CoreState& st, const ClientRequest& req, ServerResponse& resp);
