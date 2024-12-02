#ifndef SLICE_HXX
#define SLICE_HXX

#include <iostream>
#include <print>
#include <string>
#include <vector>

#include "utils.hpp"

#endif // SLICE_HXX

template<typename T, typename Container>
concept Iterable = requires(Container c) {
    requires std::is_same_v<T, typename std::decay_t<Container>::value_type>;
    { std::begin(c) } -> std::input_iterator;
    { std::end(c) } -> std::sentinel_for<decltype(std::begin(c))>;
};

/**
 * @class Slice
 * @brief A view over a dynamic, resizable collection of homogeneous elements.
 *
 * A typical `Slice` represents a view over a dynamic, resizable collection of homogeneous elements.
 * It behaves similarly to array slices in other programming languages and introduces dynamic
 * behavior, while typically serving as a view over an array-like structure. This design is inspired
 * by the slice concept in the Go language.
 *
 * @note For more information about array slicing in general, refer to
 *       [Array Slicing on Wikipedia](https://en.wikipedia.org/wiki/Array_slicing).
 *       To learn more about Go's Slice model, visit the
 *       [Go Tour on Slices](https://go.dev/tour/moretypes/7).
 *
 * @tparam T The type of elements in the slice.
 */
template<typename T>
class Slice {
private:
    T * _arr;    ///< The collection of elements in the slice.
    size_t _len; ///< The number of elements currently in the slice.
    size_t _cap; ///< The maximum capacity of the slice.
    /*-
     * The abstraction function AF is
     *      AF(_arr) = [a_0, a_1, …, a_len-1, a_len, …, a_cap]
     * where
     *      - a_0, a_1, …, a_len-1 are the effective elements
     *      - a_len, …, a_cap are inactive elements that are still alloc to avoid
     *        memory reallocations.
     * The inactive elements are not accessed but remain allocated to optimize
     * memory usage.
     *
     * The representation invariant RI states that
     *      - 0 ≤ _len < _cap
     *      - _arr = nullptr  <==>  \text{_len} = 0
     * The invariant ensures that the slice's capacity is always greater than or equal to its
     * length, and that the array pointer is `null` if the slice is empty.
     */

    static_assert(
      std::is_trivially_destructible_v<T> || std::is_destructible_v<T>,
      "Type T must provide a destructor if it is not trivially destructible"
    );

    void allocate(size_t);          ///< Allocates `this` and its elements.
    void deallocate();              ///< Deallocates `this` and its elements.
    void destroy_elems(size_t);     ///< Destroys elements of `this`.

public:
    Slice();                        ///< Default constructor.
    Slice(size_t);                  ///< Size constructor.
    Slice(T *, size_t);             ///< Array constructor.

    template<typename Container>
    requires Iterable<T, Container> // Constraint: the container must satisfy the Iterable concept
    Slice(Container && container);  ///< Templated constructor.

    template<typename... Args>
    Slice(Args &&... args);         ///< Variadic constructor.

    T * operator[](size_t);         ///< Subscript operator.
    Slice<T> operator[](size_t, size_t); ///< Slice operator.

    void print()
    {
        for (size_t i = 0; i < _len; ++i) {
            std::println("{}", _arr[i]);
        }
    }

    std::string toString()
    {
        std::string s;
        for (size_t i = 0; i < _len; ++i) {
            s += std::format("{}\n", _arr[i]);
        }
        return s;
    }

    ~Slice() noexcept; ///< Destructor.
};

/**
 * @brief Default constructor.
 *
 * Creates an empty slice.
 */
template<typename T>
Slice<T>::Slice() : _arr(nullptr), _len(0), _cap(0)
{}

/**
 * @brief Size constructor.
 *
 * Creates a slice of the given size. The elements of the collection are uninitialized.
 *
 * @param cap The initial capacity of the slice.
 */
template<typename T>
Slice<T>::Slice(size_t cap) : _arr(nullptr), _len(0), _cap(cap)
{
    allocate(cap);
}

/**
 * @brief Array constructor.
 *
 * Creates a slice taking an existing collection.
 * Specifically, it creates a slice focusing `this` view over an existing raw C-style array.
 *
 * @param brr The array to view.
 * @param size The size of `brr`.
 *
 * @throws invalid_argument if the array pointer is `nullptr` and the size is greater than zero.
 */
template<typename T>
Slice<T>::Slice(T * brr, size_t size) : _arr(brr), _len(size), _cap(size)
{
    if (brr == nullptr && size > 0) {
        throw std::invalid_argument(
          "Error: a slice cannot be nullptr if the size is greater than zero."
        ); // XXX:
    }
}

/**
 * @brief Templated constructor.
 *
 * Creates a slice taking an existing collection of elements.
 * The collection can be either copied or moved.
 * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
 *
 * @tparam Container The type of the collection.
 * @param container The container from which to generate the slice.
 *
 * @throws Any exception that may be thrown during the operation.
 */
template<typename T>
template<typename Container>
requires Iterable<T, Container>
Slice<T>::Slice(Container && container)
    : _arr(nullptr), _len(std::distance(std::begin(container), std::end(container))), _cap(_len)
{
    allocate(_cap);
    size_t i = 0;
    try {
        for (auto && el : std::forward<Container>(container)) {
            if constexpr (std::is_move_constructible_v<T>) {
                std::println("Iterable Move");
                new (_arr + i) T(std::move(el));
            } else if constexpr (std::is_copy_constructible_v<T>) {
                std::println("Iterable Copy");
                new (_arr + i) T(el);
            } else {
                static_assert(
                  std::is_constructible_v<T, decltype(el)>,
                  "Element type is neither copy-constructible nor move-constructible!"
                );
            }
            i++;
        }
    }
    catch (...) {
        destroy_elems(i);
        deallocate();
        throw;
    }
}

/**
 * @brief Variadic constructor.
 *
 * Creates a slice using multiple singular elements.
 * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
 *
 * @tparam Args The types of the elements.
 * @param args The elements to be added to the slice.
 *
 * @throws Any exception that may be thrown during the operation.
 */
template<typename T>
template<typename... Args>
Slice<T>::Slice(Args &&... args) : _arr(nullptr), _len(sizeof...(args)), _cap(_len)
{
    allocate(_cap);
    size_t i = 0;
    try {
        if constexpr (std::is_move_constructible_v<T>) {
            std::println("Variadic Move");
            ((new (_arr + i++) T(std::move(args))), ...);
        } else if constexpr (std::is_copy_constructible_v<T>) {
            std::println("Variadic Copy");
            ((new (_arr + i++) T(args)), ...);
        }
    }
    catch (...) {
        destroy_elems(i);
        deallocate();
        throw;
    }
}

/**
 * @brief Destructor.
 *
 * If the elements can be trivially destroyed, it simply deletes the collection and puts `this`
 * in a waiting phase. Otherwise, it calls the destructor for those particular elements.
 */
template<typename T>
Slice<T>::~Slice() noexcept
{
    std::println("Destruction");
    destroy_elems(_len);
    deallocate();
}

/**
 * @brief Utility function to destroy the elements of the slice.
 *
 * Destroys the elements of the slice if they are not trivially destructible.
 * The elements are destroyed in reverse order.
 *
 * @param count The number of elements to destroy.
 */
template<typename T>
void Slice<T>::destroy_elems(size_t count)
{
    if (_arr) return;
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (size_t i = 0; i < count; ++i) {
            _arr[i].~T();
        }
    }
}

/**
 * @brief Allocates memory for the slice.
 *
 * Allocates memory of the specified size and sets the view on that chunk of data.
 *
 * @param cap The capacity to allocate.
 */
template<typename T>
void Slice<T>::allocate(size_t cap)
{
    _arr = static_cast<T *>(::operator new[](cap * sizeof(T)));
}

/**
 * @brief Deallocates memory of the slice.
 *
 * Frees the memory and resets the slice to an empty state.
 */
template<typename T>
void Slice<T>::deallocate()
{
    if (_arr) {
        ::operator delete[](_arr);
        _arr = nullptr;
        _len = 0;
        _cap = 0;
    }
}

template<typename T>
T * Slice<T>::operator[](size_t i)
{
    if (i < 0 || i >= _len) {
        throw std::out_of_range("Invalid argument"); // XXX:
    }
    return &_arr[i];
}

template<typename T>
Slice<T> Slice<T>::operator[](size_t i, size_t f)
{
    if (i < 0 || f < 0 || i >= _len || f >= _len || f <= i) {
        throw std::out_of_range("Invalid argument");
    }
    return Slice<T>(&_arr[i], f - i);
}

void mjdebug()
{
    std::println("+---------- DEBUG ----------+");
}

void test_ctors();

int main()
{
    test_ctors();

    Slice<int> s(1, 3, 4, 5, 6);
    std::println("{:s}", s.toString());
    s.print();

    return 0;
}

void test_ctors()
{
    Point p;
    std::vector<Point> pp = {p};

    Slice<Point> s1(p);
    Slice<Point> s2;
    Slice<Point> s3(pp);
    std::println("{}", s3[0]->x);

    OnlyCopyable cp1(0);
    OnlyMovable mv1(0);
    std::vector<OnlyCopyable> vcp = {cp1};
    std::vector<OnlyMovable> vmv;
    vmv.emplace_back(std::move(mv1));

    Slice<OnlyCopyable> s4(cp1);
    Slice<OnlyMovable> s5(mv1);
    Slice<OnlyCopyable> s6(vcp);
    Slice<OnlyMovable> s7(vmv);
}
