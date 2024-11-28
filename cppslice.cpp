#include <ranges>
#include <type_traits>
#include <vector>

template<typename T, typename Container>
concept Iterable = requires(Container c) {
    typename Container::value_type;
    requires std::is_same_v<T, typename Container::value_type>;
    { std::begin(c) } -> std::input_iterator;
    { std::end(c) } -> std::sentinel_for<decltype(std::begin(c))>;
};

template<typename T, typename V>
concept View = std::ranges::input_range<V> && std::is_same_v<T, std::ranges::range_value_t<V>>;

template<typename T>
auto make_slice(View<T> auto && view, size_t start, size_t end) {
    if (start > end) throw std::out_of_range("Start index cannot be greater than end index");
    return view | std::ranges::views::drop(start) | std::ranges::views::take(end - start);
}

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

    void allocate(size_t);         ///< Allocates `this` and its elements.
    void deallocate();             ///< Deallocates `this` and its elements.
    void destroy_elems(size_t);    ///< Destroys elements of `this`.

public:
    Slice();                       ///< Default constructor.
    Slice(size_t);                 ///< Size constructor.
    Slice(T *, size_t);            ///< Array constructor.

    template<typename Container>
    requires std::is_same_v<T, typename std::remove_reference_t<Container>::value_type>
    Slice(Container && container); ///< Templated constructor.

    template<typename... Args>
    requires(std::is_constructible_v<T, Args &&> && ...)
    Slice(Args &&... args);        ///< Variadic constructor.

    ~Slice() noexcept;             ///< Destructor.
};

/**
 * @brief Default constructor.
 *
 * Creates an empty slice.
 */
template<typename T>
Slice<T>::Slice() : _arr(nullptr), _len(0), _cap(0) {}

/**
 * @brief Size constructor.
 *
 * Creates a slice of the given size. The elements of the collection are uninitialized.
 *
 * @param cap The initial capacity of the slice.
 */
template<typename T>
Slice<T>::Slice(size_t cap) : _arr(nullptr), _len(0), _cap(cap) {
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
Slice<T>::Slice(T * brr, size_t size) : _arr(brr), _len(size), _cap(size) {
    if (brr == nullptr && size > 0) {
        throw std::invalid_argument(
          "Error: a slice cannot be nullptr if the size is greater than zero."
        );
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
requires std::is_same_v<T, typename std::remove_reference_t<Container>::value_type>
Slice<T>::Slice(Container && container)
    : _arr(nullptr), _len(std::distance(std::begin(container), std::end(container))), _cap(_len) {
    allocate(_cap);
    size_t i = 0;
    try {
        for (auto && el : std::forward<Container>(container)) {
            new (_arr + i) T(std::forward<decltype(el)>(el));
            i++;
        }
    } catch (...) {
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
requires(std::is_constructible_v<T, Args &&> && ...)
Slice<T>::Slice(Args &&... args) : _arr(nullptr), _len(sizeof...(args)), _cap(_len) {
    allocate(_cap);
    size_t i = 0;
    try {
        ((new (_arr + i++) T(std::forward<Args>(args))), ...);
    } catch (...) {
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
Slice<T>::~Slice() noexcept {
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
void Slice<T>::destroy_elems(size_t count) {
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
void Slice<T>::allocate(size_t cap) {
    _arr = static_cast<T *>(::operator new[](cap * sizeof(T)));
}

/**
 * @brief Deallocates memory of the slice.
 *
 * Frees the memory and resets the slice to an empty state.
 */
template<typename T>
void Slice<T>::deallocate() {
    if (_arr) {
        ::operator delete[](_arr);
        _arr = nullptr;
        _len = 0;
        _cap = 0;
    }
}

int main() {
    std::vector<int> v1 = {2, 3, 4, 5, 6};
    Slice<int> s1(v1);
    Slice<int> s2(std::move(v1));
    Slice<int> s3(22, 23, 24, 25, 26);
}
