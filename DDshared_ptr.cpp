#include <iostream>
#include <utility>

namespace DD {
// 引用计数类
class ref_count {
public:
    int use_count() const noexcept { return count_; }

    int inc_ref() noexcept { return ++count_; }

    int dec_ref() noexcept { return --count_; }

private:
    int count_ = 1;
};

// shared_ptr类
template<class T>
class shared_ptr {
public:
    constexpr shared_ptr() noexcept = default;

    constexpr explicit shared_ptr(std::nullptr_t) noexcept: shared_ptr() {}

    explicit shared_ptr(T *ptr) : ptr_(ptr) {
        if (ptr) {
            rep_ = new ref_count();
        }
    }

    shared_ptr(const shared_ptr &rhs) noexcept: ptr_(rhs.ptr), rep_(rhs.rep_) {
        if (rep_) {
            rep_->inc_ref();
        }
    }

    shared_ptr(shared_ptr &&rhs) noexcept: ptr_(rhs.ptr_), rep_(rhs.rep_) {
        rhs.ptr_ = nullptr;
        rhs.rep_ = nullptr;
    }

    ~shared_ptr() noexcept {
        if (rep_ != nullptr && rep_->dec_ref() == 0) {
            delete ptr_;
            delete rep_;
        }
    }

    shared_ptr &operator=(const shared_ptr &rhs) {
        shared_ptr(rhs).swap(*this);
        return *this;
    }

    shared_ptr &operator=(shared_ptr &&rhs) {
        shared_ptr(std::move(rhs)).swap(*this);
        return *this;
    }

    void swap(shared_ptr &rhs) noexcept {
        std::swap(ptr_, rhs.ptr_);
        std::swap(rep_, rhs.rep_);
    }

    void reset(T *ptr) { shared_ptr(ptr).swap(*this); }

    void reset(std::nullptr_t) noexcept { reset(); };

    T *get() const noexcept { return *ptr_; }

    T &operator*() const noexcept { return *ptr_; }

    T *operator->() const noexcept { return ptr_; }

    int use_count() const noexcept { return rep_ != nullptr ? rep_->use_count() : 0; }

    bool unique() const noexcept { return use_count() == 1; }

    explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }

private:
    T *ptr_ = nullptr;
    ref_count *rep_ = nullptr;
};

template<class T, class... Args>
auto make_shared(Args &&... args) {
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // namespace DD


// 测试
#include <vector>
using namespace DD;

void f(shared_ptr<int> p, int *a) {}

int main() {
    /**
     *下面的参数中，执行顺序是不确定的。
     * 如果执行顺序为 1. new int(1024) 2. int* a = new int(10) 3. shared_ptr<int>()
     * 那么如果在第 2 步出现了异常，第 3 步就不会执行，那么第 1 步分配的指针就永远都不会被释放
     * 可以用 make_shared 函数解决
     */
    f(shared_ptr<int>(new int(1024)), new int(10));
    f(make_shared<int>(1024), new int(10));

    make_shared<std::vector<int>>(3, 3);
}















