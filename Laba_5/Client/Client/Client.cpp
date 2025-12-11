#include <windows.h>
#include <iostream>
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
    DWORD     pid;
};

struct ServerResponse {
    Status    status;
    employee  data;
};

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\EmpServer";

bool sendRecv(HANDLE hPipe, const ClientRequest& req, ServerResponse& resp) {
    DWORD written = 0;
    if (!WriteFile(hPipe, &req, sizeof(req), &written, nullptr) || written != sizeof(req)) {
        std::cerr << "WriteFile failed, err=" << GetLastError() << "\n";
        return false;
    }
    DWORD readBytes = 0;
    if (!ReadFile(hPipe, &resp, sizeof(resp), &readBytes, nullptr) || readBytes != sizeof(resp)) {
        std::cerr << "ReadFile failed, err=" << GetLastError() << "\n";
        return false;
    }
    return true;
}

int main() {
    // Подключение к серверу
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    while (true) {
        hPipe = CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hPipe != INVALID_HANDLE_VALUE) break;

        if (GetLastError() != ERROR_PIPE_BUSY) {
            std::cerr << "Could not open pipe, err=" << GetLastError() << "\n";
            return 1;
        }
        if (!WaitNamedPipeW(PIPE_NAME, 5000)) {
            std::cerr << "WaitNamedPipe timeout.\n";
            return 1;
        }
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &mode, nullptr, nullptr);

    std::cout << "Client started. Commands: read, mod, exit\n";

    std::string cmd;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, cmd);

        if (cmd == "exit") {
            ClientRequest req{ OP_EXIT, 0, {}, GetCurrentProcessId() };
            ServerResponse resp{};
            sendRecv(hPipe, req, resp);
            break;
        }
        else if (cmd == "read") {
            int key;
            std::cout << "ID: ";
            std::cin >> key;
            std::cin.ignore();

            ClientRequest req{ OP_READ_START, key, {}, GetCurrentProcessId() };
            ServerResponse resp{};
            if (!sendRecv(hPipe, req, resp)) continue;

            if (resp.status == ST_OK) {
                std::cout << "Record: " << resp.data.num << " " << resp.data.name << " " << resp.data.hours << "\n";
                req.op = OP_READ_DONE;
                sendRecv(hPipe, req, resp);
            }
            else if (resp.status == ST_ERR_NOT_FOUND) {
                std::cout << "Not found.\n";
            }
            else {
                std::cout << "Busy.\n";
            }
        }
        else if (cmd == "mod") {
            int key;
            std::cout << "ID: ";
            std::cin >> key;
            std::cin.ignore();

            ClientRequest req{ OP_WRITE_LOCK, key, {}, GetCurrentProcessId() };
            ServerResponse resp{};
            if (!sendRecv(hPipe, req, resp)) continue;

            if (resp.status != ST_OK) {
                std::cout << "Lock failed (busy/not found).\n";
                continue;
            }
            std::cout << "Current: " << resp.data.num << " " << resp.data.name << " " << resp.data.hours << "\n";

            employee e = resp.data;
            std::string nm;
            std::cout << "New name: ";
            std::cin >> nm;
            std::cin.ignore();

            // безопасное копирование имени без <cstring>
            for (size_t j = 0; j < sizeof(e.name); ++j) {
                if (j < nm.size()) e.name[j] = nm[j];
                else e.name[j] = '\0';
            }

            std::cout << "New hours: ";
            std::cin >> e.hours;
            std::cin.ignore();

            req.op = OP_WRITE_COMMIT;
            req.data = e;
            if (!sendRecv(hPipe, req, resp)) {
                req.op = OP_WRITE_UNLOCK;
                sendRecv(hPipe, req, resp);
                continue;
            }

            if (resp.status == ST_OK) std::cout << "Updated.\n";
            else std::cout << "Commit error.\n";

            req.op = OP_WRITE_UNLOCK;
            sendRecv(hPipe, req, resp);
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    CloseHandle(hPipe);
    return 0;
}
