#pragma once
#include <concepts>
#include <type_traits>
#include <vector>

#ifndef nil
#define nil nullptr
#endif

template<typename T, typename Container>
concept ContainerOfT = requires(Container container) {
    typename std::remove_cvref_t<Container>::value_type;
    requires std::is_same_v<typename std::remove_cvref_t<Container>::value_type, T>;
    { container.size() } /*-> std::convertible_to<std::size_t>*/;
    { container.begin() };
    { container.end() };
};

template<typename T>
class Slice {
public:
    T * _arr;

private:
    size_t _size;
    size_t _capacity;
    bool _hold;

    void allocate(size_t);

    static_assert(std::is_trivially_destructible_v<T> || std::is_destructible_v<T>,
                  "Type T must provide a destructor if it is not trivially destructible");

public:
    Slice();
    Slice(size_t);

    template<typename Container>
    requires ContainerOfT<T, Container>
    Slice(Container &&);

    template<typename... Args>
    requires(std::is_constructible_v<T, Args &&> && ...)
    Slice(Args &&... args);

    ~Slice();
};

template<typename T>
Slice<T>::Slice() : _arr(nil), _size(0), _capacity(0), _hold(true) {}

template<typename T>
Slice<T>::Slice(size_t cap) : _arr(nil), _size(0), _capacity(cap), _hold(true) {
    allocate(cap);
}

template<typename T>
template<typename Container>
requires ContainerOfT<T, Container>
Slice<T>::Slice(Container && container)
    : _arr(nil), _size(container.size()), _capacity(_size + 1), _hold(true) {
    allocate(_capacity);
    size_t i = 0;
    for (auto && elem : container) {
        _arr[i++] = std::forward<decltype(elem)>(elem);
    }
}

template<typename T>
template<typename... Args>
requires(std::is_constructible_v<T, Args &&> && ...)
Slice<T>::Slice(Args &&... args)
    : _arr(nil), _size(sizeof...(args)), _capacity(_size), _hold(true) {
    allocate(_capacity);
    size_t i = 0;
    ((_arr[i++] = T(std::forward<Args>(args))), ...);
}

template<typename T>
Slice<T>::~Slice() {
    if (_hold && _arr != nil) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < _size; ++i) {
                _arr[i].~T();
            }
        }
        delete[] _arr;
        _arr = nil;
    }
}

template<typename T>
void Slice<T>::allocate(size_t cap) {
    if (_arr == nil) {
        _arr  = new T[cap];
        _hold = true;
    }
}

int main() {
    std::vector<int> v1 = {2, 3, 4, 5, 6};
    Slice<int> s1(v1);
    Slice<int> s2(std::move(v1));
    Slice<int> s3(22, 23, 24, 25, 26);
}
