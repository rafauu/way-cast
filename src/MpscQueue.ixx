export module MpscQueue;

import std;

export template <typename T>
class LockFreeMpscQueue {
public:
    LockFreeMpscQueue() : head_(new Node()), tail_(head_.load(std::memory_order_relaxed)) {}

    ~LockFreeMpscQueue() {
        while (Node* const old_head = head_.load(std::memory_order_relaxed)) {
            head_.store(old_head->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delete old_head;
        }
    }

    LockFreeMpscQueue(const LockFreeMpscQueue&) = delete;
    LockFreeMpscQueue& operator=(const LockFreeMpscQueue&) = delete;

    void push(T value) {
        auto new_node = std::make_unique<Node>(std::move(value));
        Node* const new_tail = new_node.release();
        Node* const prev_tail = tail_.exchange(new_tail, std::memory_order_acq_rel);
        prev_tail->next.store(new_tail, std::memory_order_release);
    }

    std::optional<T> pop() {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next.load(std::memory_order_acquire);

        if (!next) {
            return std::nullopt;
        }

        T result = std::move(next->data);
        head_.store(next, std::memory_order_relaxed);
        delete head;
        return result;
    }

private:
    struct Node {
        T data{};
        std::atomic<Node*> next{nullptr};
        Node() = default;
        explicit Node(T val) : data(std::move(val)) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
};
