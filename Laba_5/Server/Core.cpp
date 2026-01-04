#include "pch.h"
#include "Core.h"
#include <fstream>

int findIndexById(const CoreState& st, int key) {
    for (size_t i = 0; i < st.recs.size(); ++i)
        if (st.recs[i].num == key) return static_cast<int>(i);
    return -1;
}

CoreState::Lock* getLock(CoreState& st, int key) {
    for (auto& kv : st.locks) if (kv.first == key) return &kv.second;
    st.locks.push_back({ key, CoreState::Lock{} });
    return &st.locks.back().second;
}

bool writeRecordToFile(const CoreState& st, int idx) {
    std::fstream file(st.fileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return false;
    file.seekp(static_cast<std::streamoff>(idx) * static_cast<std::streamoff>(sizeof(employee)), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&st.recs[idx]), sizeof(employee));
    return true;
}

void handleRequestCore(CoreState& st, const ClientRequest& req, ServerResponse& resp) {
    if (req.op == OP_EXIT) { resp.status = ST_OK; return; }

    int idx = findIndexById(st, req.num);
    if (idx < 0) { resp.status = ST_ERR_NOT_FOUND; return; }

    auto* lock = getLock(st, req.num);

    switch (req.op) {
    case OP_READ_START:
        if (lock->writerLocked) { resp.status = ST_ERR_BUSY; break; }
        lock->readers++;
        resp.status = ST_OK;
        resp.data = st.recs[idx];
        break;

    case OP_READ_DONE:
        if (lock->readers <= 0) { resp.status = ST_ERR_PROTOCOL; break; }
        lock->readers--;
        resp.status = ST_OK;
        break;

    case OP_WRITE_LOCK:
        if (lock->writerLocked || lock->readers > 0) { resp.status = ST_ERR_BUSY; break; }
        lock->writerLocked = true;
        resp.status = ST_OK;
        resp.data = st.recs[idx];
        break;

    case OP_WRITE_COMMIT:
        if (!lock->writerLocked) { resp.status = ST_ERR_PROTOCOL; break; }
        st.recs[idx] = req.data;
        resp.status = ST_OK;
        break;

    case OP_WRITE_UNLOCK:
        if (!lock->writerLocked) { resp.status = ST_ERR_PROTOCOL; break; }
        lock->writerLocked = false;
        resp.status = ST_OK;
        break;

    default:
        resp.status = ST_ERR_PROTOCOL;
    }
}
