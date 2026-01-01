#include <iostream>
#include <vector>

#include "Factorial.h"
#include "Unique.h"
#include "LinkedList.h"

int main() {
    using std::cout;
    using std::cin;
    using std::vector;

    try {

        cout << "=== 1. First n factorials ===\n";
        cout << "Enter n (a natural number): ";

        std::size_t n{};
        if (!(cin >> n)) {
            cout << "Invalid input for n\n";
            return 1;
        }

        try {
            auto facts = math::Factorial::generate(n);
            cout << "Factorials:\n";
            for (std::size_t i = 0; i < facts.size(); ++i) {
                cout << (i + 1) << "! = " << facts[i] << '\n';
            }
        }
        catch (const std::invalid_argument& ex) {
            cout << "Error: " << ex.what() << '\n';
        }
        catch (const std::overflow_error& ex) {
            cout << "Overflow: " << ex.what() << '\n';
        }

        cout << "\n";


        cout << "=== 2. Remove duplicates from an array ===\n";
        cout << "Enter the number of elements: ";

        std::size_t m{};
        if (!(cin >> m)) {
            cout << "Invalid input for array size\n";
            return 1;
        }

        vector<int> data;
        data.reserve(m);

        cout << "Enter " << m << " integers:\n";
        for (std::size_t i = 0; i < m; ++i) {
            int x{};
            if (!(cin >> x)) {
                cout << "Invalid input for array element\n";
                return 1;
            }
            data.push_back(x);
        }

        auto unique = util::removeDuplicates(data);
        cout << "Array without duplicates (order preserved):\n";
        for (int v : unique) {
            cout << v << ' ';
        }
        cout << "\n\n";

        cout << "=== 3. Reverse a singly linked list (recursively) ===\n";
        cout << "Enter the number of list elements: ";

        std::size_t k{};
        if (!(cin >> k)) {
            cout << "Invalid input for list size\n";
            return 1;
        }

        ds::LinkedList list;

        cout << "Enter " << k << " integers:\n";
        for (std::size_t i = 0; i < k; ++i) {
            int x{};
            if (!(cin >> x)) {
                cout << "Invalid input for list element\n";
                return 1;
            }
            list.push_back(x);
        }

        cout << "Original list: ";
        for (int v : list.toVector()) {
            cout << v << ' ';
        }
        cout << '\n';

        list.reverseRecursive();

        cout << "Reversed list: ";
        for (int v : list.toVector()) {
            cout << v << ' ';
        }
        cout << '\n';
    }
    catch (const std::exception& ex) {
        std::cout << "Unhandled exception: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
