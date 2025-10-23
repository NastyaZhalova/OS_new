#include "listreverse.h"
#include <unordered_set>
#include <stdexcept>

Node* ReverseListRecursive(Node* head, std::unordered_set<Node*>* visited, int depth) {
    constexpr int MAX_RECURSION_DEPTH = 10'000;

    if (depth > MAX_RECURSION_DEPTH) {
        throw std::overflow_error("Maximum recursion depth exceeded. Possible cycle in list.");
    }

    if (head && visited) {
        if (!visited->insert(head).second) {
            throw std::logic_error("Cycle detected in linked list.");
        }
    }

    if (!head || !head->next) {
        return head;
    }

    Node* newHead = ReverseListRecursive(head->next, visited, depth + 1);
    head->next->next = head;
    head->next = nullptr;
    return newHead;
}

Node* ReverseListRecursive(Node* head) {
    std::unordered_set<Node*> visited;
    return ReverseListRecursive(head, &visited, 0);
}
