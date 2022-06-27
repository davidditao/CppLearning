#include <iostream> // size_t
#include <utility> // std::exchage>

namespace DD {
template<class T>
class vector {
public:
    // 跟分配内存无关的函数应该为 noexcept
    vector() noexcept = default;

    // explicit不允许隐式类型转换
    explicit vector(size_t n) : cap_(n), ptr_(alloc(cap_)) {
        for (; size_ < n; ++size_) {
            construct(ptr_ + size_);
        }
    }

    vector(size_t n, const T &x) : cap_(n), ptr_(alloc(cap_)) {
        for (; size_ < n; ++size_) {
            construct(ptr_ + size_, x);
        }
    }

    // 拷贝构造
    vector(const vector &rhs) : cap_(rhs.size()), ptr_(rhs.ptr_) {
        for (; size_ < rhs.size(); ++size_) {
            construct(ptr_ + size_, rhs[size_]);
        }
    }

    // 移动构造
    vector(vector &&rhs) noexcept {
//        cap_ = rhs.cap_;
//        rhs.cap_ = 0;
        cap_ = std::exchange(rhs.cap_, 0);
        size_ = std::exchange(rhs.size_, 0);
        ptr_ = std::exchange(rhs.ptr_, nullptr);
    }

    // 初始化列表
    vector(std::initializer_list<T> il) : cap_(il.size()), ptr_(alloc(cap_)) {
        for (auto &x : il) {
            construct(ptr_ + size_, x);
            ++size_;
        }
    }

    ~vector() noexcept {
        clear();
        dealloc(ptr_);
    }

    void swap(vector &rhs) noexcept {
//        T* tmp = rhs.ptr_;
//        rhs.ptr_ = ptr_;
//        ptr_ = tmp;
        using std::swap;
        swap(cap_, rhs.cap_);
        swap(size_, rhs.size_);
        swap(ptr_, rhs.ptr_);
    }

    void clear() noexcept {
        for (; size_ > 0; --size_) {
            destroy(ptr_ + size_ - 1);
        }
    }

    // 拷贝赋值
    vector &operator==(const vector &rhs) {
        if (this != &rhs) {
            // 先拷贝构造一个临时的 vector, 将它与当前的vector交换。这个临时的vector会在出作用域的时候进行析构
            // 好处：如果在构造的时候发生异常，原来的 vector 不会受到影响
            vector(rhs).swap(*this);
        }
        return *this;
    }

    // 移动赋值
    vector &operator==(vector &&rhs) noexcept {
        if (this != &rhs) {
            // 同理：先移动构造一个临时的vector，再将它与当前的vector交换
            // 虽然rhs参数传进来的是右值，但是这个形参rhs是有名字的，所以还是个左值，所以还要用std::move转换为右值
            vector(std::move(rhs)).swap(*this);
        }
        return *this;
    }

    // 初始化列表赋值
    vector &operator==(std::initializer_list<T> il) {
        // 同理
        vector(il).swap(*this);
        return *this;
    }

    void push_back(const T &x) {
        emplace_back(x);
    }

    // 右值
    void push_back(T &&x) {
        emplace_back(std::move(x));
    }

    template<class... Args>
    // 可变参模版
    void emplace_back(Args &&... args) {
        if (size_ == cap_) {
            // 扩容：2倍
            auto new_cap = cap_ != 0 ? cap_ * 2 : 1;
            auto new_ptr = alloc(new_cap);
            // 拷贝/移动旧元素
            for (auto new_size = 0; new_size < size_; ++new_size) {
                // move_if_noexcept 只有在异常的时候才会移动，否则执行拷贝操作
                construct(new_ptr + new_size, std::move_if_noexcept(ptr_[new_size]));
            }
            cap_ = new_cap;
            ptr_ = new_ptr;
        }
        // 添加新元素
        construct(ptr_ + size_, std::forward<Args>(args)...);
        ++size_;
    }

    void pop_back() noexcept {
        destroy(ptr_ + size_ - 1);
    }

    // 加上 const 类型，不然 const vector 对象不能访问这个成员方法
    size_t size() const noexcept { return size_; }

    size_t capacity() const noexcept { return cap_; }

    bool empty() const noexcept { return size_ == 0; }

    T &operator[](size_t i) { return ptr_[i]; }

    const T &operator[](size_t i) const { return ptr_[i]; }

    T *begin() noexcept { return ptr_; }

    T *end() noexcept { return ptr_ + size_; }

    const T *begin() const noexcept { return ptr_; }

    const T *end() const noexcept { return ptr_ + size_; }

private:
    // 分配内存
    T *alloc(size_t n) {
        // 堆中分配内存，调用 operator new
        return static_cast<T *>(::operator new(sizeof(T) * n));
    }

    // 释放内存
    void dealloc(T *p) noexcept {
        ::operator delete(p);
    }

    // 元素构造
    template<class... Args>
    void construct(T *p, Args &&... args) {
        // 调用构造函数, 调用 placement new
        ::new(p) T(std::forward<Args>(args)...); // 调用 T 的构造函数，使用完美转发
    }

    // 元素析构
    void destroy(T *p) noexcept {
        p->~T();
    }

    size_t cap_ = 0;
    size_t size_ = 0;
    T *ptr_ = nullptr; // 存放在堆中
};

template<class T>
void swap(vector<T> &a, vector<T> &b) {
    a.swap(b);
}
}; // namespace DD



// 测试
#include <string>
#include <vector>
//using namespace std;
using namespace DD;

// 三种 swap
// 1. 拷贝
template<class T>
void swap1(T &a, T &b) {
    auto tmp = a;
    a = b;
    b = tmp;
}

// 2. 移动
template<class T>
void swap2(T &a, T &b) {
    auto tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

// 3. 调用 T 类的 swap
template<class T>
void swap3(T &a, T &b) {
    a.swap(b);
}

int main() {
    auto print = [](vector<std::string> &v) {
        std::cout << v.size() << ":" << v.capacity();
        for (const auto &x : v) {
            std::cout << " " << x;
        }
        std::cout << "\n";
    };

    // 1. 列表
    vector<std::string> v{"1", "2", "3"};
    print(v);

    // 2. 扩容
    v.clear();
    for (char c = 'a'; c <= 'z'; c++) {
        print(v);
//        v.push_back(std::string(1, c));
        v.emplace_back(1, c);
    }

    // 3. swap
    vector<int> a, b;
    using std::swap;
    swap(a, b);

    return 0;
}












