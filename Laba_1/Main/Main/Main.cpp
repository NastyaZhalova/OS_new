#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include "employee.h"

void printBinaryFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open binary file.\n";
        return;
    }

    employee emp;
    std::cout << "\nBinary file contents:\n";
    while (in.read(reinterpret_cast<char*>(&emp), sizeof(emp))) {
        std::cout << "ID: " << emp.num
            << ", Name: " << emp.name
            << ", Hours: " << emp.hours << "\n";
    }
}

void printTextFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Failed to open report file.\n";
        return;
    }

    std::cout << "\nReport contents:\n";
    std::string line;
    while (std::getline(in, line)) {
        std::cout << line << "\n";
    }
}

bool runProcess(const std::wstring& commandLine) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    wchar_t* cmd = _wcsdup(commandLine.c_str());

    BOOL success = CreateProcessW(
        NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    free(cmd);

    if (!success) {
        std::cerr << "Failed to launch process.\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int main() {
    std::string binFile;
    int count;

    std::cout << "Enter the name of the binary file: ";
    std::cin >> binFile;
    std::cout << "Enter the number of employee records: ";
    std::cin >> count;

    std::wstring creatorCmd = L"Creator.exe " +
        std::wstring(binFile.begin(), binFile.end()) + L" " +
        std::to_wstring(count);

    if (!runProcess(creatorCmd)) return 1;

    printBinaryFile(binFile);

    std::string reportFile;
    double rate;
    std::cout << "\nEnter the name of the report file: ";
    std::cin >> reportFile;
    std::cout << "Enter the hourly wage: ";
    std::cin >> rate;

    std::wstring reporterCmd = L"Reporter.exe " +
        std::wstring(binFile.begin(), binFile.end()) + L" " +
        std::wstring(reportFile.begin(), reportFile.end()) + L" " +
        std::to_wstring(rate);

    if (!runProcess(reporterCmd)) return 1;

    printTextFile(reportFile);

    return 0;
}
