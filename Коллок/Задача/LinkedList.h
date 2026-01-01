#pragma once
#include <vector>

namespace ds {

    class LinkedList {
    public:
        struct Node {
            int value;
            Node* next;
            Node(int v, Node* n = nullptr) : value(v), next(n) {}
        };

        LinkedList() = default;
        LinkedList(std::initializer_list<int> init);
        ~LinkedList();

        void push_back(int v);
        void reverseRecursive();

        std::vector<int> toVector() const;

    private:
        Node* head_ = nullptr;

        static Node* reverseImpl(Node* node);
        void clear();
    };

}
