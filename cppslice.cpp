#pragma once
#include <concepts>
#include <print>

#ifndef nil
#define nil nullptr
#endif

template<typename T, typename Container>
concept ContainerOfT = requires(Container container) {
    typename Container::value_type;
    requires std::is_same_v<typename Container::value_type, T>;
    { container.size() };
    { container.begin() };
};

template<typename T>
class Slice {
public:
    T * _arr;

private:
    bool _hold;
    size_t _size;
    size_t _capacity;

    void allocate(size_t);

public:
    Slice();
    Slice(size_t);

    template<ContainerOfT<T> Container>
    Slice(Container &&);

    ~Slice();
};

template<typename T>
Slice<T>::Slice() : _arr(nullptr), _hold(true), _size(0), _capacity(0) {}

template<typename T>
Slice<T>::Slice(size_t cap) : _hold(true), _size(0), _capacity(cap) {
    allocate(cap);
}

template<typename T>
template<ContainerOfT<T> Container>
Slice<T>::Slice(Container && container) : _size(container.size()), _capacity(_size + 1) {
    allocate(_capacity);
    if constexpr (std::is_move_assignable_v<T>) {
        size_t i = 0;
        for (const auto & elem : container) {
            _arr[i] = std::move(container[i]);
            i++;
        }
    } else if constexpr (std::is_copy_assignable_v<T>) {
        std::fprintf(stderr, "Warning: T is not move-assignable: falling back to copies.");
        size_t i = 0;
        for (const auto & elem : container) {
            _arr[i++] = elem;
        }
    } else {
        static_assert(std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>,
                      "Error: T must should be move-assignable or at least copy-assignable");
    }
}

template<typename T>
Slice<T>::~Slice() { // XXX:
    if (_hold && _arr) {
        if constexpr (std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < _size; i++) {
                _arr[i].~T();
            }
        }
        delete[] _arr;
        _arr = nil;                   // NB
    }
}

template<typename T>
void Slice<T>::allocate(size_t cap) { // XXX:
    if (!_hold) {
        _arr  = new T[cap];
        _hold = true;
        _arr  = nil;
    }
}
