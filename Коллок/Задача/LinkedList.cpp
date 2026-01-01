#include "LinkedList.h"

namespace ds {

    LinkedList::LinkedList(std::initializer_list<int> init) {
        for (int v : init) push_back(v);
    }

    LinkedList::~LinkedList() {
        clear();
    }

    void LinkedList::push_back(int v) {
        Node* n = new Node(v);
        if (!head_) {
            head_ = n;
            return;
        }
        Node* cur = head_;
        while (cur->next) cur = cur->next;
        cur->next = n;
    }

    void LinkedList::reverseRecursive() {
        head_ = reverseImpl(head_);
    }

    LinkedList::Node* LinkedList::reverseImpl(Node* node) {
        if (!node || !node->next)
            return node;

        Node* newHead = reverseImpl(node->next);
        node->next->next = node;
        node->next = nullptr;
        return newHead;
    }

    std::vector<int> LinkedList::toVector() const {
        std::vector<int> out;
        Node* cur = head_;
        while (cur) {
            out.push_back(cur->value);
            cur = cur->next;
        }
        return out;
    }

    void LinkedList::clear() {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
        head_ = nullptr;
    }

}
