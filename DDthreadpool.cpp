#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace DD {
class thread_pool {
public:
    explicit thread_pool(size_t n) : max_queue_size_(n), running_(false) {}

    ~thread_pool() { stop(); }

    void start(size_t thread_num) {
        if (running_) return;

        running_ = true;
        threads_.reserve(thread_num);  // 给容器预先分配空间
        for (int i = 0; i < thread_num; ++i) {
            threads_.emplace_back(&thread_pool::worker, this);
        }
    }

    // 停止线程池
    void stop() {
        if (!running_) return;

        {
            std::lock_guard<std::mutex> lk(m_);

            running_ = false;

            // 通知所有线程
            not_full_.notify_all();
            not_empty_.notify_all();
        }

        // 回收所有线程
        for (auto &t : threads_) {
            if (t.joinable()) t.join();
        }
    }

    // 生产者线程
    template <class Fun>
    void submit(Fun f) {
        std::unique_lock<std::mutex> lk(m_);
        not_full_.wait(lk, [this] { return !running_ || !is_full(); });

        if (!running_) throw;
        assert(!is_full());

        q_.push_back(std::move(f));  // 使用移动
        not_empty_.notify_one();
    }

private:
    bool is_full() {
        return max_queue_size_ > 0 && q_.size() >= max_queue_size_;
    }

    // 消费者线程
    void worker() {
        while (true) {
            task t;
            {
                std::unique_lock<std::mutex> lk(m_);
                not_empty_.wait(lk,
                                [this] { return !running_ || !q_.empty(); });

                if (!running_) return;
                assert(!q_.empty());

                t = std::move(q_.front());
                q_.pop_front();
                not_full_.notify_one();
            }  // 释放 mutex
            // 由于 t()
            // 的执行需要耗费时间，而且它的执行是不需要加锁的，所以执行之前要先释放锁
            t();
        }
    }

    using task = std::function<void()>;
    std::vector<std::thread> threads_;  // 保存创建好的线程
    std::deque<task> q_;                // 任务队列
    std::mutex m_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    size_t max_queue_size_;
    bool running_;  // 标记线程池是否正在运行
};

};  // namespace DD

// 测试
#include <iostream>

using namespace DD;

int main() {
    thread_pool pool(3);
    pool.start(3);

    for (int i = 0; i < 100; ++i) {
        pool.submit([i] {
            // printf("%d: [%d]\n", std::this_thread::get_id(), i);
            std::cout << std::this_thread::get_id() << ": [" << i << "]" << std::endl;
        });
    }

    return 0;
}