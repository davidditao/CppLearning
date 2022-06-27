#include <iostream>

#include <thread> // 线程库

void func01(int a) {

    std::cout << "Hello sub thread!\n";
    // 休眠 50 ms, chrono 是 C++11 中的时间库
    std::this_thread::sleep_for(std::chrono::microseconds(50));
}

/*---------------------------- 一、创建线程 --------------------------------*/
void test01() {
    int a = 0;
    std::thread thread1(func01, a); // 创建线程：函数名 + 参数

    std::cout << "Hello main thread!\n";
    // 获取子线程 id
    std::cout << "sub thread id is: " << thread1.get_id();
    std::cout << std::endl;
    // 获取当前线程的id
    std::cout << "main thread id is: " << std::this_thread::get_id();
    std::cout << std::endl;

    // 如果 主线程 比 子线程 先结束，解决方法有两个：回收 或 分离
    thread1.join(); // 回收子线程
//    thread1.detach(); // 分离子线程
}

/*---------------------------- 二、互斥量 --------------------------------*/
#include <mutex> // 互斥量

// 1. mutex lock
std::mutex mtx;
int global_vaiable = 0;

void func02() {
    for (int i = 0; i < 10000000; i++) {
        mtx.lock();
        global_vaiable++;
        global_vaiable--;
        mtx.unlock();
    }
}

void test02() {
    std::thread t1(func02);
    std::thread t2(func02);

    t1.join();
    t2.join();

    std::cout << "current value is: " << global_vaiable << std::endl;
}

// 2. 封装类 std::lock_guard ：可以避免死锁问题
void func03() {
    for (int i = 0; i < 10000000; i++) {
        mtx.lock();
        global_vaiable++;
        if (global_vaiable == 1) {
            // 如果程序在中途离开了，那么如果没有释放锁，就会造成死锁问题
            break;
        }
        global_vaiable--;
        mtx.unlock();
    }

    for (int i = 0; i < 10000000; i++) {
        /**
         * 如果使用 lock_guard，那么在它离开作用域的时候，
         * 会执行析构函数，其中会执行解锁操作
         */
        std::lock_guard<std::mutex> lock(mtx);
        global_vaiable++;
        if (global_vaiable == 1) {
            break;
        }
        global_vaiable--;
    }

}

void test03() {
    std::thread t1(func03);
    std::thread t2(func03);

    t1.join();
    t2.join();

    std::cout << "current value is: " << global_vaiable << std::endl;
}

// 3. std::lock() 对多个互斥量上锁，避免死锁问题
std::mutex mtx1;
std::mutex mtx2;

void func04() {
    for (int i = 0; i < 10000000; i++) {
        // func04 和 func05 同时执行会造成死锁
        mtx1.lock();
        mtx2.lock();
        global_vaiable++;
        global_vaiable--;
        mtx1.unlock();
        mtx2.unlock();
    }

    for (int i = 0; i < 10000000; i++) {
        /**
         * 使用 std::lock，可以对多个互斥量上锁
         * 避免上面这种死锁问题
         */
        std::lock(mtx1, mtx2);
        global_vaiable++;
        global_vaiable--;

    }

}

void func05() {
    for (int i = 0; i < 10000000; i++) {
        mtx2.lock();
        mtx1.lock();
        global_vaiable++;
        global_vaiable--;
        mtx2.unlock();
        mtx1.unlock();
    }
}

void test04() {
    std::thread t1(func04);
    std::thread t2(func05);

    t1.join();
    t2.join();

    std::cout << "current value is: " << global_vaiable << std::endl;
}

// 4. std::unique_lock 类
void test05() {
    /*
     * lock_guard 和 unique_lock 有什么区别呢？
     */
    // 1. lock_guard 只能在出作用域的时候才能解锁，而 unique_lock 可以中途解锁
    {
        // lock_guard
        std::lock_guard<std::mutex> lock1(mtx);
        // unique_lock
        std::unique_lock<std::mutex> lock2(mtx);
        // unique_lock 解锁
        lock2.unlock();
    } // lock_guard 解锁

    // 2. unique_lock 还可以记录当前锁的状态
    std::unique_lock<std::mutex> lock(mtx);
    if (lock) {
        std::cout << "is lock!\n";
    }

    // 3. unique_lock 可以尝试加锁
    lock.try_lock();
}

/*---------------------------- 三、原子变量 --------------------------------*/
#include <atomic>

std::atomic<int> globalv(0); // 定义原子变量

void func06() {
    for (int i = 0; i < 10000000; i++) {
        /**
         * 不需要加锁就能实现线程安全
         * 具体实现取决于具体的标准库
         */
        globalv++;
        globalv--;
    }
}

/*---------------------------- 四、条件变量 --------------------------------*/
// 生产者消费者问题
#include <deque>

std::mutex mutex;
std::deque<int> q;

/**
 * 使用 while(true) 是十分消耗 CPU 资源的, 我们可以用条件变量来解决
 */
std::condition_variable cv;

void productor() {
    int cnt = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);

        q.push_back(cnt);

        // 唤醒一个线程
        cv.notify_one();
        // 唤醒所有线程
//        cv.notify_all();

        if (cnt < 9999999) {
            cnt++;
        } else {
            cnt = 0;
        }
    }
}

void consumer() {
    int data = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);

        // 条件变量的使用
        if (q.empty()) {
            // 这里传入 lock 的意思是：如果不满足条件，那么就释放锁，并进入休眠状态
            cv.wait(lock);
        }

        if (!q.empty()) {
            data = q.front();
            q.pop_front();
            std::cout << "get: " << data << std::endl;
        }
    }
}

void test06() {
    std::thread t1(productor);
    std::thread t2(consumer);

    t1.join();
    t2.join();
}

// 问题：虚假唤醒！
/**
 * 如果有两个消费者线程 c1, c2，那么当第一个消费者线程 c1 被唤醒后，会正常执行。
 * 当 c1 已经执行完一轮后，继续执行下一轮循环。此时生产者线程 p 写入一个数据，又会唤醒一个线程。
 * 由于队列中有生产者写入的一个数据，那么 c1 不会进入休眠。
 * 那么此时就会导致，有两个线程正在工作，并且两个线程在争夺一个数据。
 */
// 解决方法：
void func07() {
    std::unique_lock<std::mutex> lock(mtx);
    /*
     * 这里将 if 改为 while，这样当 c2 被唤醒的时候，会重新检查一遍队列是否为空，
     * 此时，c1 早已经将队列中的数据取完了, c2 会继续休眠，不会跟 c1 继续竞争了
     */
    while (q.empty()) {
        cv.wait(lock);
    }
}

int main() {
//    test01();
//    test02();
//    test05();
    test06();
}
