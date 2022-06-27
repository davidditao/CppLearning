#include <deque>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <optional> // 作为 try_pop 的返回值

namespace DD {
template<class T>
class Queue {    // 有界队列
public:
    Queue(size_t capacity) : max_queue_size_(capacity) {}

    template<class M>
    void push(M &&val) {    // 阻塞

        std::unique_lock lk(m_);
        not_full_.wait(lk, [this] { return !is_full(); });

        assert(!is_full());

        q_.push_back(std::forward<M>(val));
        not_empty_.notify_one();
    }

    T pop() {    // 阻塞

        std::unique_lock lk(m_);
        not_empty_.wait(lk, [this] { return !q_.empty(); });

        assert(!q_.empty());

        T ret{std::move_if_noexcept(q_.front())};
        q_.pop_front();
        not_full_.notify_one();
        return ret;
    }

    template<class M>
    bool try_push(M &&val) {    // 非阻塞

        std::lock_guard lk(m_);
        if (is_full()) return false;

        q_.push_back(std::forward<M>(val));
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> try_pop() {    // 非阻塞

        std::lock_guard lk(m_);
        if (q_.empty()) return {};

        std::optional<T> ret{std::move_if_noexcept(q_.front())};
        q_.pop_front();
        not_full_.notify_one();
        return ret;
    }

    bool is_full() {
        return max_queue_size_ > 0 && q_.size() >= max_queue_size_;
    }

private:
    std::deque<T> q_;
    std::mutex m_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    int max_queue_size_;
};
};

// 测试
#include <thread>
#include <iostream>
#include <queue>
using namespace DD;

int main() {
    Queue<int> q(10);
//    std::queue<int> q;
//    std::thread t1(
//        [&] {
//            for (int i = 0; i < 100; i++) {
//                q.push(i);
//            }
//        }
//    );
//
//    std::thread t2(
//        [&] {
//            for (int i = 0; i < 100; i++) {
//                std::cout << q.pop() << " ";
//            }
//        }
//    );

    // try_...
    std::thread t3(
        [&] {
            for (int i = 0; i < 100;) {
                if (q.try_push(i)) {
                    i++;
                }
            }
        });

    std::thread t4(
        [&] {
            for (int i = 0; i < 100;) {
                if (auto ret = q.try_pop()) {
                    std::cout << *ret << " ";
                    i++;
                }
            }
        }
    );

//    t1.join();
//    t2.join();
    t3.join();
    t4.join();
    return 0;
}