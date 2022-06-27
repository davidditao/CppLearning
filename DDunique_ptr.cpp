#include <utility>

namespace DD {
template<class T>
class unique_ptr {
public:
    // constexpr：编译期的时候就得到结果，提高运行时的效率
    constexpr unique_ptr() noexcept = default;

    explicit constexpr unique_ptr(std::nullptr_t) noexcept: unique_ptr() {}

    explicit unique_ptr(T *ptr) noexcept: ptr_(ptr) {}

    // 不允许拷贝构造
    unique_ptr(const unique_ptr &rhs) = delete;

    // 移动构造
    unique_ptr(unique_ptr &&rhs) noexcept: ptr_(rhs.release()) {}

    ~unique_ptr() noexcept { delete ptr_; }

    // 不允许拷贝赋值
    unique_ptr &operator=(const unique_ptr &rhs) = delete;

    // 如果赋一个空指针是可以的
    constexpr unique_ptr &operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    // 移动赋值
    unique_ptr &operator=(unique_ptr &&rhs) noexcept {
        // 获得 rhs 的指针，并为当前 unique_ptr 设置该指针
        reset(rhs.release());
        return *this;
    }

    // 得到裸指针
    T *get() const noexcept { return ptr_; }

    // 释放管理的指针，并返回这个指针
    T *release() noexcept { return std::exchange(ptr_, nullptr); }

    // 更换管理的指针，并将原来的指针释放
    void reset(T *ptr) noexcept { delete std::exchange(ptr_, ptr); }

    // 交换
    void swap(unique_ptr &rhs) noexcept { std::swap(ptr_, rhs.ptr_); }

    // 注意：如果对空指针解引用会抛出异常
    T &operator*() const { return *ptr_; }

    T *operator->() const noexcept { return ptr_; }

    // 将指针转为bool类型
    explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }

private:
    T *ptr_ = nullptr;
};

template<class T, class... Args>
auto make_unique(Args &&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}; // namespace DD

// 测试
#include <vector>
using namespace DD;

int main() {

    const unique_ptr<int> p1;
    unique_ptr<const int> p2;

    auto p = make_unique<int>(10);

    make_unique<std::vector<int>>(3, 3);

    return 0;
}
