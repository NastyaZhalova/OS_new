#include <iostream>
#include <fstream>
#include <string>
#include "employee.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: Creator <filename> <count>\n";
        return 1;
    }

    const char* filename = argv[1];
    int count = std::stoi(argv[2]);

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open file for writing.\n";
        return 1;
    }

    for (int i = 0; i < count; ++i) {
        employee emp;
        std::cout << "Enter employee #" << (i + 1) << ":\n";
        std::cout << "ID: "; std::cin >> emp.num;
        std::cout << "Name: "; std::cin >> emp.name;
        std::cout << "Hours: "; std::cin >> emp.hours;
        out.write(reinterpret_cast<char*>(&emp), sizeof(emp));
    }

    out.close();
    return 0;
}
