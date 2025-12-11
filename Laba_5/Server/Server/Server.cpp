#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>

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
    DWORD     pid;
};

struct ServerResponse {
    Status    status;
    employee  data;
};

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\EmpServer";
std::vector<employee> gRecs;
std::wstring gFileName;

struct LockState { int readers = 0; DWORD writer = 0; };
std::mutex gLocksMx;
std::vector<std::pair<int, LockState>> gLocks;

LockState* getLock(int key) {
    for (auto& kv : gLocks) if (kv.first == key) return &kv.second;
    gLocks.push_back({ key, LockState{} });
    return &gLocks.back().second;
}

int findIndexById(int key) {
    for (size_t i = 0; i < gRecs.size(); ++i) if (gRecs[i].num == key) return (int)i;
    return -1;
}

bool writeRecordToFile(int idx) {
    std::fstream file(gFileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return false;
    file.seekp(idx * sizeof(employee), std::ios::beg);
    file.write(reinterpret_cast<char*>(&gRecs[idx]), sizeof(employee));
    return true;
}

void printFile() {
    std::cout << "\nFile dump:\n";
    for (auto& e : gRecs) {
        std::cout << e.num << " " << e.name << " " << e.hours << "\n";
    }
}

void handleRequest(const ClientRequest& req, ServerResponse& resp) {
    std::lock_guard<std::mutex> lk(gLocksMx);
    int idx = findIndexById(req.num);
    if (idx < 0) { resp.status = ST_ERR_NOT_FOUND; return; }
    LockState* lock = getLock(req.num);

    switch (req.op) {
    case OP_READ_START:
        if (lock->writer != 0) resp.status = ST_ERR_BUSY;
        else { lock->readers++; resp.status = ST_OK; resp.data = gRecs[idx]; }
        break;
    case OP_READ_DONE:
        if (lock->readers > 0) { lock->readers--; resp.status = ST_OK; }
        else resp.status = ST_ERR_PROTOCOL;
        break;
    case OP_WRITE_LOCK:
        if (lock->readers == 0 && lock->writer == 0) {
            lock->writer = req.pid; resp.status = ST_OK; resp.data = gRecs[idx];
        }
        else resp.status = ST_ERR_BUSY;
        break;
    case OP_WRITE_COMMIT:
        if (lock->writer == req.pid) {
            gRecs[idx] = req.data; writeRecordToFile(idx); resp.status = ST_OK;
        }
        else resp.status = ST_ERR_PROTOCOL;
        break;
    case OP_WRITE_UNLOCK:
        if (lock->writer == req.pid) { lock->writer = 0; resp.status = ST_OK; }
        else resp.status = ST_ERR_PROTOCOL;
        break;
    case OP_EXIT: resp.status = ST_OK; break;
    default: resp.status = ST_ERR_PROTOCOL;
    }
}

HANDLE createPipeInstance() {
    return CreateNamedPipeW(
        PIPE_NAME, PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr
    );
}

void serveClient(HANDLE hPipe) {
    if (!ConnectNamedPipe(hPipe, nullptr)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) { CloseHandle(hPipe); return; }
    }
    ClientRequest req{}; ServerResponse resp{ ST_OK };
    DWORD readBytes = 0;
    while (ReadFile(hPipe, &req, sizeof(req), &readBytes, nullptr)) {
        if (readBytes != sizeof(req)) break;
        handleRequest(req, resp);
        DWORD written = 0;
        if (!WriteFile(hPipe, &resp, sizeof(resp), &written, nullptr)) break;
        if (req.op == OP_EXIT) break;
    }
    DisconnectNamedPipe(hPipe); CloseHandle(hPipe);
}

bool spawnClients(int count) {
    for (int i = 0; i < count; ++i) {
        STARTUPINFOW si{ sizeof(si) }; PROCESS_INFORMATION pi{};
        // путь к клиенту: рядом с сервером
        if (!CreateProcessW(
            L"client.exe", nullptr,
            nullptr, nullptr,
            FALSE,
            CREATE_NEW_CONSOLE,   // <--- вот этот флаг
            nullptr, nullptr,
            &si, &pi))
        {
            std::cerr << "Failed to start client " << (i + 1) << ", err=" << GetLastError() << "\n";
            return false;
        }

        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    }
    return true;
}

int main() {
    std::cout << "Enter file name: ";
    std::string fname; std::getline(std::cin, fname);
    gFileName = std::wstring(fname.begin(), fname.end());

    std::cout << "Enter number of employees: ";
    int recCount; std::cin >> recCount; std::cin.ignore();
    gRecs.resize(recCount);

    for (int i = 0; i < recCount; ++i) {
        int id; std::string nm; double h;
        std::cout << "num name hours: ";
        std::cin >> id >> nm >> h;
        gRecs[i].num = id;
        for (size_t j = 0; j < sizeof(gRecs[i].name); ++j) {
            if (j < nm.size()) gRecs[i].name[j] = nm[j];
            else gRecs[i].name[j] = '\0';
        }
        gRecs[i].hours = h;
    }

    // запись в бинарный файл
    {
        std::ofstream out(gFileName, std::ios::binary);
        out.write(reinterpret_cast<char*>(gRecs.data()), gRecs.size() * sizeof(employee));
    }

    printFile();

    std::cout << "Enter number of clients: ";
    int clients; std::cin >> clients; std::cin.ignore();
    spawnClients(clients);

    // создаём пайпы и обслуживаем клиентов параллельно
    std::vector<std::thread> workers;
    for (int i = 0; i < clients; ++i) {
        HANDLE hPipe = createPipeInstance();
        workers.emplace_back([hPipe]() { serveClient(hPipe); });
    }

    std::cout << "Server ready. Type 'quit' to stop.\n";
    std::string cmd;
    while (true) { std::getline(std::cin, cmd); if (cmd == "quit") break; }

    for (auto& t : workers) if (t.joinable()) t.join();

    printFile();
    return 0;
}
