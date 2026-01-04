#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <array>

using boost::asio::ip::tcpConsoleApplication1;

#pragma pack(push,1)
struct employee {
    int num;
    char name[10];
    double hours;
};
enum OpCode { OP_READ_START, OP_READ_DONE, OP_WRITE_LOCK, OP_WRITE_COMMIT, OP_WRITE_UNLOCK, OP_EXIT };
enum Status { ST_OK, ST_ERR_NOT_FOUND, ST_ERR_BUSY, ST_ERR_PROTOCOL };
struct ClientRequest {
    OpCode op;
    int num;
    employee data;
    unsigned pid;
};
struct ServerResponse {
    Status status;
    employee data;
};
#pragma pack(pop)

struct LockState { int readers = 0; bool writerLocked = false; };

struct ServerState {
    std::vector<employee> recs;
    std::wstring fileNameW;
    std::unordered_map<int, LockState> locks;
    std::mutex mtx;
};

static int findIndexById(const ServerState& st, int key) {
    for (size_t i = 0; i < st.recs.size(); ++i)
        if (st.recs[i].num == key) return static_cast<int>(i);
    return -1;
}

static LockState& getLock(ServerState& st, int key) { return st.locks[key]; }

static bool writeRecordToFile(const ServerState& st, int idx) {
    if (st.fileNameW.empty()) return true;
    std::string fileName(st.fileNameW.begin(), st.fileNameW.end());
    std::fstream file(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) return false;
    auto off = static_cast<std::streamoff>(idx) * static_cast<std::streamoff>(sizeof(employee));
    file.seekp(off, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&st.recs[idx]), sizeof(employee));
    return true;
}

static void printFile(const ServerState& st, const char* header) {
    std::cout << header << "\n";
    for (const auto& e : st.recs) {
        std::cout << e.num << " " << e.name << " " << e.hours << "\n";
    }
}

static void handleRequest(ServerState& st, const ClientRequest& req, ServerResponse& resp) {
    std::lock_guard<std::mutex> g(st.mtx);

    if (req.op == OP_EXIT) { resp.status = ST_OK; return; }

    int idx = findIndexById(st, req.num);
    if (idx < 0) { resp.status = ST_ERR_NOT_FOUND; return; }

    auto& lock = getLock(st, req.num);

    switch (req.op) {
    case OP_READ_START:
        if (lock.writerLocked) { resp.status = ST_ERR_BUSY; break; }
        lock.readers++;
        resp.status = ST_OK;
        resp.data = st.recs[idx];
        break;
    case OP_READ_DONE:
        if (lock.readers <= 0) { resp.status = ST_ERR_PROTOCOL; break; }
        lock.readers--;
        resp.status = ST_OK;
        break;
    case OP_WRITE_LOCK:
        if (lock.writerLocked || lock.readers > 0) { resp.status = ST_ERR_BUSY; break; }
        lock.writerLocked = true;
        resp.status = ST_OK;
        resp.data = st.recs[idx];
        break;
    case OP_WRITE_COMMIT:
        if (!lock.writerLocked) { resp.status = ST_ERR_PROTOCOL; break; }
        st.recs[idx] = req.data;
        writeRecordToFile(st, idx);
        resp.status = ST_OK;
        break;
    case OP_WRITE_UNLOCK:
        if (!lock.writerLocked) { resp.status = ST_ERR_PROTOCOL; break; }
        lock.writerLocked = false;
        resp.status = ST_OK;
        break;
    default:
        resp.status = ST_ERR_PROTOCOL;
    }
}

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, ServerState& state) : socket_(std::move(socket)), st_(state) {}
    void start() { readRequest(); }

private:
    tcp::socket socket_;
    ServerState& st_;
    std::array<char, sizeof(ClientRequest)> inbuf_{};
    std::array<char, sizeof(ServerResponse)> outbuf_{};

    void readRequest() {
        auto self = shared_from_this();
        boost::asio::async_read(socket_, boost::asio::buffer(inbuf_),
            [this, self](boost::system::error_code ec, std::size_t bytes) {
                if (ec || bytes != sizeof(ClientRequest)) return;
                ClientRequest req{};
                std::memcpy(&req, inbuf_.data(), sizeof(ClientRequest));
                ServerResponse resp{};
                handleRequest(st_, req, resp);
                std::memcpy(outbuf_.data(), &resp, sizeof(ServerResponse));
                writeResponse(req.op == OP_EXIT);
            });
    }

    void writeResponse(bool closing) {
        auto self = shared_from_this();
        boost::asio::async_write(socket_, boost::asio::buffer(outbuf_),
            [this, self, closing](boost::system::error_code ec, std::size_t bytes) {
                if (ec || bytes != sizeof(ServerResponse)) return;
                if (closing) { socket_.close(); return; }
                readRequest();
            });
    }
};

class Server {
public:
    Server(boost::asio::io_context& io, unsigned short port, ServerState& st)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)), st_(st) {
        accept();
    }

private:
    tcp::acceptor acceptor_;
    ServerState& st_;

    void accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) std::make_shared<Session>(std::move(socket), st_)->start();
            accept();
            });
    }
};

static bool spawnClients(int count) {
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
        if (!CreateProcessW(clientPath.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE,
            nullptr, nullptr, &si, &pi))
        {
            std::wcerr << L"Failed to start client " << (i + 1) << L", err=" << GetLastError() << L"\n";
            return false;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return true;
}

int main() {
    ServerState st;

    std::cout << "Enter file name: ";
    std::string fname;
    std::getline(std::cin, fname);
    st.fileNameW = std::wstring(fname.begin(), fname.end());

    std::cout << "Enter number of employees: ";
    int recCount;
    while (!(std::cin >> recCount) || recCount <= 0) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Ошибка: введите положительное число сотрудников: ";
    }
    std::cin.ignore();
    st.recs.resize(static_cast<size_t>(recCount));

    for (int i = 0; i < recCount; ++i) {
        int id;
        std::string nm;
        double h;

        std::cout << "num name hours: ";

        if (!(std::cin >> id)) {
            std::cin.clear(); std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: ID должен быть числом.\n"; --i; continue;
        }
        if (!(std::cin >> nm)) {
            std::cin.clear(); std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: имя должно быть строкой.\n"; --i; continue;
        }
        if (!(std::cin >> h)) {
            std::cin.clear(); std::cin.ignore(10000, '\n');
            std::cout << "Ошибка: часы должны быть числом.\n"; --i; continue;
        }

        st.recs[i].num = id;
        for (size_t j = 0; j < sizeof(st.recs[i].name); ++j)
            st.recs[i].name[j] = (j < nm.size()) ? nm[j] : '\0';
        st.recs[i].hours = h;
    }
    std::cin.ignore();

    {
        std::ofstream out(fname, std::ios::binary);
        out.write(reinterpret_cast<const char*>(st.recs.data()),
            static_cast<std::streamsize>(st.recs.size() * sizeof(employee)));
    }

    printFile(st, "Initial file:");

    std::cout << "Enter number of clients: ";
    int clients;
    std::cin >> clients;
    std::cin.ignore();

    try {
        boost::asio::io_context io;
        Server server(io, 5555, st);
        spawnClients(clients);
        std::thread ioThread([&] { io.run(); });

        std::cout << "Server ready. Type 'quit' to stop.\n";
        std::string cmd;
        while (true) {
            std::getline(std::cin, cmd);
            if (cmd == "quit") break;
        }

        io.stop();
        ioThread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }

    printFile(st, "Final file:");
    return 0;
}
