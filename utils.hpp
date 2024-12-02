#ifndef UTILS_HXX
#define UTILS_HXX
class CopyPoint {
public:
    CopyPoint() = default;
    ~CopyPoint() = default;
    CopyPoint(CopyPoint &&) = delete;
    CopyPoint(const CopyPoint &) = default;
};

class MovePoint {
public:
    MovePoint() = default;
    ~MovePoint() = default;
    MovePoint(MovePoint &&) = default;
    MovePoint(MovePoint &) = delete;
};

struct Point {
    int x = 0;
    int y = 0;
};

class OnlyMovable {
public:
    OnlyMovable(int value) : value_(value) {}
    OnlyMovable(OnlyMovable && other) noexcept : value_(other.value_)
    {
        other.value_ = 0;
    }
    OnlyMovable & operator=(OnlyMovable && other) noexcept
    {
        if (this != &other) {
            value_ = other.value_;
            other.value_ = 0;
        }
        return *this;
    }
    OnlyMovable(const OnlyMovable &) = delete;
    OnlyMovable & operator=(const OnlyMovable &) = delete;

private:
    int value_;
};

class OnlyCopyable {
public:
    OnlyCopyable(int value) : value_(value) {}
    OnlyCopyable(const OnlyCopyable & other) : value_(other.value_) {}
    OnlyCopyable & operator=(const OnlyCopyable & other)
    {
        if (this != &other) {
            value_ = other.value_;
        }
        return *this;
    }
    OnlyCopyable(OnlyCopyable &&) = delete;
    OnlyCopyable & operator=(OnlyCopyable &&) = delete;

private:
    int value_;
};

#endif // UTILS_HXX
