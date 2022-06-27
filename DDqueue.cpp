// 线程安全的队列：解决生产者消费者问题
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cassert>  // assert
#include <optional> // optional 做为 try_pop 的返回值 -> Cpp17

namespace DD {
template<class T>
class Queue { // 无界队列：队列没有容量
public:
    /**
     * std::mutex 和 std::condition_variable 不支持拷贝和赋值操作，
     * 所以我们的队列也应该不支持拷贝和赋值操作。
     * 但是编译器会替我们判断，所以下面两句可以不用写
     */
//        Queue(const Queue&) = delete;
//        Queue& operator=(Queue&) = delete;

    void push(const T &val) { emplace(val); }

    void push(T &&val) { emplace(std::move(val)); }

    template<class... Args>
    void emplace(Args &&... args) {
        // 加锁
        std::lock_guard<std::mutex> lk(m_);
        // 添加元素
        q_.emplace(std::forward<Args>(args)...);
        // 唤醒一个消费者线程
        cv_.notify_one();
    } // 释放锁

    // 阻塞
    T pop() {
        // 加锁
        std::unique_lock<std::mutex> lk(m_);
        // 判断队列是否不为空，不为空，
        cv_.wait(lk, [this] { return !q_.empty(); });

        assert(!q_.empty());

        T ret(std::move_if_noexcept(q_.front())); // 使用移动来获取首元素
        q_.pop();

        return ret;
    }

    // 非阻塞
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.empty()) {
            // 如果
            return {};
        }

        std::optional ret(std::move_if_noexcept(q_.front())); // 使用移动来获取首元素
        q_.pop();

        return ret;
    }

private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
};
}; // namespace DD


// 测试
#include <thread>
#include <iostream>

using namespace DD;

int main() {
    Queue<int> q;

    std::thread t1(
        [&] {
            for (int i = 0; i < 100; i++) {
                q.push(i);
            }
        }
    );

    std::thread t2(
        [&] {
            for (int i = 0; i < 100; i++) {
//                    std::cout << q.pop() << " ";
                if (auto ret = q.try_pop()) {
                    std::cout << *ret << " ";
                }
            }
        }
    );

    t1.join();
    t2.join();
    return 0;
}