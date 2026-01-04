#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <limits>
#include <array>
#include <cstring>

using boost::asio::ip::tcp;

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

static bool sendRecv(tcp::socket& sock, const ClientRequest& req, ServerResponse& resp) {
    boost::asio::write(sock, boost::asio::buffer(&req, sizeof(req)));
    std::array<char, sizeof(ServerResponse)> inbuf{};
    size_t n = boost::asio::read(sock, boost::asio::buffer(inbuf));
    if (n != sizeof(ServerResponse)) return false;
    std::memcpy(&resp, inbuf.data(), sizeof(ServerResponse));
    return true;
}

int main() {
    try {
        boost::asio::io_context io;
        tcp::socket sock(io);
        sock.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 5555));

        std::cout << "Client started. Commands: read, mod, exit\n";

        std::string cmd;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, cmd)) break;

            if (cmd == "exit") {
                ClientRequest req{};
                req.op = OP_EXIT; req.num = 0; req.pid = 0; req.data = employee{};
                ServerResponse resp{};
                sendRecv(sock, req, resp);
                break;
            }
            else if (cmd == "read") {
                int key;
                std::cout << "ID: ";
                if (!(std::cin >> key)) {
                    std::cin.clear(); std::cin.ignore(10000, '\n');
                    std::cout << "Некорректный ID.\n";
                    continue;
                }
                std::cin.ignore();

                ClientRequest req{};
                req.op = OP_READ_START; req.num = key; req.pid = 0; req.data = employee{};
                ServerResponse resp{};
                if (!sendRecv(sock, req, resp)) { std::cout << "I/O error.\n"; continue; }

                if (resp.status == ST_OK) {
                    std::cout << "Record: " << resp.data.num << " " << resp.data.name << " " << resp.data.hours << "\n";
                    req.op = OP_READ_DONE;
                    sendRecv(sock, req, resp);
                }
                else if (resp.status == ST_ERR_NOT_FOUND) {
                    std::cout << "Not found.\n";
                }
                else if (resp.status == ST_ERR_BUSY) {
                    std::cout << "Busy.\n";
                }
                else {
                    std::cout << "Protocol error.\n";
                }
            }
            else if (cmd == "mod") {
                int key;
                std::cout << "ID: ";
                if (!(std::cin >> key)) {
                    std::cin.clear(); std::cin.ignore(10000, '\n');
                    std::cout << "Некорректный ID.\n";
                    continue;
                }
                std::cin.ignore();

                ClientRequest req{};
                req.op = OP_WRITE_LOCK; req.num = key; req.pid = 0; req.data = employee{};
                ServerResponse resp{};
                if (!sendRecv(sock, req, resp)) { std::cout << "I/O error.\n"; continue; }

                if (resp.status != ST_OK) {
                    if (resp.status == ST_ERR_NOT_FOUND) std::cout << "Not found.\n";
                    else std::cout << "Lock failed (busy).\n";
                    continue;
                }

                std::cout << "Current: " << resp.data.num << " " << resp.data.name << " " << resp.data.hours << "\n";

                employee e = resp.data;
                std::string nm;
                std::cout << "New name: ";
                if (!std::getline(std::cin, nm)) {
                    std::cout << "Некорректное имя.\н";
                    req.op = OP_WRITE_UNLOCK;
                    sendRecv(sock, req, resp);
                    continue;
                }
                for (size_t j = 0; j < sizeof(e.name); ++j)
                    e.name[j] = (j < nm.size()) ? nm[j] : '\0';

                std::cout << "New hours: ";
                if (!(std::cin >> e.hours)) {
                    std::cin.clear(); std::cin.ignore(10000, '\n');
                    std::cout << "Некорректный ввод числа.\n";
                    req.op = OP_WRITE_UNLOCK;
                    sendRecv(sock, req, resp);
                    continue;
                }
                std::cin.ignore();

                req.op = OP_WRITE_COMMIT;
                req.data = e;
                if (!sendRecv(sock, req, resp)) {
                    req.op = OP_WRITE_UNLOCK;
                    sendRecv(sock, req, resp);
                    continue;
                }

                if (resp.status == ST_OK) std::cout << "Updated.\n";
                else std::cout << "Commit error.\n";

                req.op = OP_WRITE_UNLOCK;
                sendRecv(sock, req, resp);
            }
            else {
                std::cout << "Unknown command.\n";
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
