#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>


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


struct LockState {
    HANDLE rw_mutex;   
    HANDLE mutex;      
    LONG   readCount;
};



std::vector<employee> gRecs;
std::wstring gFileName;
std::vector<std::pair<int, LockState>> gLocks;
HANDLE gMapSem; 

LockState* getLock(int key) {
    WaitForSingleObject(gMapSem, INFINITE);
    for (size_t i = 0; i < gLocks.size(); ++i) {
        if (gLocks[i].first == key) {
            ReleaseSemaphore(gMapSem, 1, nullptr);
            return &gLocks[i].second;
        }
    }
    LockState ls{};
    ls.rw_mutex = CreateSemaphoreW(nullptr, 1, 1, nullptr);
    ls.mutex = CreateSemaphoreW(nullptr, 1, 1, nullptr);
    ls.readCount = 0;
    gLocks.push_back(std::make_pair(key, ls));
    ReleaseSemaphore(gMapSem, 1, nullptr);
    return &gLocks.back().second;
}

int findIndexById(int key) {
    for (size_t i = 0; i < gRecs.size(); ++i)
        if (gRecs[i].num == key) return static_cast<int>(i);
    return -1;
}

bool writeRecordToFile(int idx) {
    std::fstream file(gFileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return false;
    file.seekp(static_cast<std::streamoff>(idx) * static_cast<std::streamoff>(sizeof(employee)), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&gRecs[idx]), sizeof(employee));
    return true;
}

void printFile() {
    std::cout << "\nFile dump:\n";
    for (size_t i = 0; i < gRecs.size(); ++i) {
        const employee& e = gRecs[i];
        std::cout << e.num << " " << e.name << " " << e.hours << "\n";
    }
}

void handleRequest(const ClientRequest& req, ServerResponse& resp) {
    int idx = findIndexById(req.num);
    if (req.op == OP_EXIT) { resp.status = ST_OK; return; }
    if (idx < 0) { resp.status = ST_ERR_NOT_FOUND; return; }

    LockState* lock = getLock(req.num);

    switch (req.op) {
    case OP_READ_START: {
        WaitForSingleObject(lock->mutex, INFINITE);
        lock->readCount++;
        if (lock->readCount == 1)
            WaitForSingleObject(lock->rw_mutex, INFINITE);
        ReleaseSemaphore(lock->mutex, 1, nullptr);
        resp.status = ST_OK;
        resp.data = gRecs[idx];
        break;
    }
    case OP_READ_DONE: {
        WaitForSingleObject(lock->mutex, INFINITE);
        if (lock->readCount > 0) {
            lock->readCount--;
            if (lock->readCount == 0)
                ReleaseSemaphore(lock->rw_mutex, 1, nullptr);
            resp.status = ST_OK;
        }
        else {
            resp.status = ST_ERR_PROTOCOL;
        }
        ReleaseSemaphore(lock->mutex, 1, nullptr);
        break;
    }
    case OP_WRITE_LOCK: {
        WaitForSingleObject(lock->rw_mutex, INFINITE);
        resp.status = ST_OK;
        resp.data = gRecs[idx];
        break;
    }
    case OP_WRITE_COMMIT: {
        gRecs[idx] = req.data;
        writeRecordToFile(idx);
        resp.status = ST_OK;
        break;
    }
    case OP_WRITE_UNLOCK: {
        ReleaseSemaphore(lock->rw_mutex, 1, nullptr);
        resp.status = ST_OK;
        break;
    }
    default:
        resp.status = ST_ERR_PROTOCOL;
    }
}

DWORD WINAPI serveClient(LPVOID param) {
    HANDLE hPipe = static_cast<HANDLE>(param);
    if (!ConnectNamedPipe(hPipe, nullptr)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            CloseHandle(hPipe);
            return 0; 
        }
    }
    ClientRequest req{};
    ServerResponse resp{ ST_OK };
    DWORD readBytes = 0;

    while (ReadFile(hPipe, &req, sizeof(req), &readBytes, nullptr)) {
        if (readBytes != sizeof(req)) break;
        handleRequest(req, resp);
        DWORD written = 0;
        if (!WriteFile(hPipe, &resp, sizeof(resp), &written, nullptr)) break;
        if (req.op == OP_EXIT) break;
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return 0;
}

HANDLE createPipeInstance() {
    return CreateNamedPipeW(
        PIPE_NAME, PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr
    );
}

bool spawnClients(int count) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring exePath(path);
    size_t pos = exePath.find_last_of(L"\\/");
    std::wstring dir = exePath.substr(0, pos + 1);
    std::wstring clientPath = dir + L"client.exe";

    for (int i = 0; i < count; ++i) {
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(
            clientPath.c_str(),
            nullptr, nullptr, nullptr,
            FALSE, CREATE_NEW_CONSOLE,
            nullptr, nullptr,
            &si, &pi))
        {
            std::cerr << "Failed to start client " << (i + 1)
                << ", err=" << GetLastError() << "\n";
            return false;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return true;
}

int main() {
    gMapSem = CreateSemaphoreW(nullptr, 1, 1, nullptr);

    std::cout << "Enter file name: ";
    std::string fname;
    std::getline(std::cin, fname);
    gFileName = std::wstring(fname.begin(), fname.end());

    std::cout << "Enter number of employees: ";
    int recCount;
    while (!(std::cin >> recCount) || recCount <= 0) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Ошибка: введите положительное число сотрудников: ";
    }
    std::cin.ignore();
    gRecs.resize(static_cast<size_t>(recCount));

    for (int i = 0; i < recCount; ++i) {
        int id;
        std::string nm;
        double h;

        std::cout << "num name hours: ";

        if (!(std::cin >> id)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: ID должен быть числом.\n";
            --i;
            continue;
        }

        if (!(std::cin >> nm)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: имя должно быть строкой.\n";
            --i;
            continue;
        }

        if (!(std::cin >> h)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: часы должны быть числом.\n";
            --i;
            continue;
        }

        gRecs[i].num = id;
        for (size_t j = 0; j < sizeof(gRecs[i].name); ++j)
            gRecs[i].name[j] = (j < nm.size()) ? nm[j] : '\0';
        gRecs[i].hours = h;
    }
    std::cin.ignore();

    {
        std::ofstream out(gFileName, std::ios::binary);
        out.write(reinterpret_cast<const char*>(gRecs.data()),
            static_cast<std::streamsize>(gRecs.size() * sizeof(employee)));
    }

    printFile();

    std::cout << "Enter number of clients: ";
    int clients;
    std::cin >> clients;
    std::cin.ignore();
    spawnClients(clients);

    for (int i = 0; i < clients; ++i) {
        HANDLE hPipe = createPipeInstance();
        DWORD tid = 0;
        CreateThread(nullptr, 0, serveClient, hPipe, 0, &tid);
    }

    std::cout << "Server ready. Type 'quit' to stop.\n";
    std::string cmd;
    while (true) {
        std::getline(std::cin, cmd);
        if (cmd == "quit") break;
    }

    printFile();
    return 0;
}
