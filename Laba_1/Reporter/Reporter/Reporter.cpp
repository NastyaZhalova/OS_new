#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include "employee.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: Reporter <input_file> <report_file> <hourly_rate>\n";
        return 1;
    }

    const char* input_file = argv[1];
    const char* report_file = argv[2];
    double rate = std::stod(argv[3]);

    std::ifstream in(input_file, std::ios::binary);
    std::ofstream out(report_file);

    if (!in || !out) {
        std::cerr << "Error opening files.\n";
        return 1;
    }

    out << "Report based on file \"" << input_file << "\"\n";
    out << "----------------------------------------------\n";
    out << std::left << std::setw(10) << "ID"
        << std::setw(12) << "Name"
        << std::setw(10) << "Hours"
        << std::setw(10) << "Salary" << "\n";

    employee emp;
    while (in.read(reinterpret_cast<char*>(&emp), sizeof(emp))) {
        double salary = emp.hours * rate;
        out << std::left << std::setw(10) << emp.num
            << std::setw(12) << emp.name
            << std::setw(10) << emp.hours
            << std::setw(10) << salary << "\n";
    }

    in.close();
    out.close();
    return 0;
}
