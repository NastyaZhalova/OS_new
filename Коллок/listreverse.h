#pragma once

struct Node {
    int value;
    Node* next;
    Node(int val) : value(val), next(nullptr) {}
};

Node* ReverseListRecursive(Node* head);
