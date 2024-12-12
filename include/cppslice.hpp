#ifndef SLICE_HXX
#define SLICE_HXX

#include <print>
#include <string>
#include <vector>

template<typename T, typename CollT>
concept Iterable = requires(CollT c) {
    requires std::is_same_v<T, typename std::decay_t<CollT>::value_type>;
    { std::begin(c) } -> std::input_iterator;
    { std::end(c) } -> std::sentinel_for<decltype(std::begin(c))>;
};

/**
 * @class Slice
 * @brief A view over a dynamic, resizable collection of homogeneous elements.
 *
 * A typical `Slice` represents a view over a dynamic, resizable collection of
 * homogeneous elements. It behaves similarly to array slices in other programming languages and
 * introduces dynamic behavior, while typically serving as a view over an array-like structure. This
 * design is inspired by the slice concept in the Go language.
 *
 * @note For more information about array slicing in general, refer to
 *       [Array Slicing on Wikipedia](https://en.wikipedia.org/wiki/Array_slicing).
 *       To learn more about Go's Slice model, visit the
 *       [Go Tour on Slices](https://go.dev/tour/moretypes/7).
 *
 * @tparam T The type of elements in the `Slice`.
 */
template<typename T>
class Slice {
private:
    T * arr_;    ///< The collection of elements in `this`.
    size_t len_; ///< The number of elements currently in `this`.
    size_t cap_; ///< The maximum capacity of `this`.
    /*–
     * AF(arr_) = [a_0, a_1, …, a_len-1, a_len, …, a_cap]
     * a_0, a_1, …, a_len-1 are the stored elements.
     * a_len, …, a_cap are inactive elements that are over-allocated.
     *
     * RI(arr_, len_, cap_) = 0 ≤ len_ < cap_ && arr_ == nullptr  <==>  len_ == 0
     */

    static_assert(
      std::is_trivially_destructible_v<T> || std::is_destructible_v<T>,
      "Type T must provide a destructor if it is not trivially destructible"
    );

    /**
     * @brief Allocates memory for `this`.
     *
     * Allocates memory of the specified size and sets the view on that chunk of data.
     */
    void allocate() {
        arr_ = static_cast<T *>(::operator new[](cap_ * sizeof(T)));
    }

    /**
     * @brief Deallocates memory of `this`.
     *
     * Frees the memory and resets `this` to an empty state.
     */
    void deallocate() {
        if (arr_) {
            ::operator delete[](arr_);
            arr_ = nullptr;
            len_ = 0;
            cap_ = 0;
        }
    }

    /**
     * @brief Utility function to destroy the elements of `this`.
     *
     * Destroys the elements of `this` if they are not trivially destructible.
     * The elements are destroyed in reverse order.
     *
     * @param count The number of elements to destroy.
     */
    void destroy_elems(size_t count) {
        if (arr_) return;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < count; ++i) { arr_[i].~T(); }
        }
    }

public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty `this`.
     */
    Slice() : arr_(nullptr), len_(0), cap_(0) {}

    /**
     * @brief Size constructor.
     *
     * Creates `this` of the given size. The elements of the collection are uninitialized.
     *
     * @param cap The initial capacity of `this`.
     */
    Slice(size_t cap) : arr_(nullptr), len_(0), cap_(cap) {
        allocate();
    }

    /**
     * @brief Array constructor.
     *
     * Creates `this` taking an existing collection.
     * Specifically, it creates `this` focusing its view over an existing raw C-style array.
     *
     * @param brr The array to view.
     * @param size The size of `brr`.
     *
     * @throws invalid_argument if the array pointer is `nullptr` and the size is greater than zero.
     */
    Slice(T * brr, size_t size) : arr_(brr), len_(size), cap_(size) {
        if (brr == nullptr && size > 0) {
            throw std::invalid_argument(
              "Error: a slice cannot be nullptr if the size is greater than zero."
            );
        }
    }

    /**
     * @brief Iterable constructor.
     *
     * Creates `this` taking an existing collection of elements.
     * The collection can be either copied or moved.
     * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
     *
     * @tparam CollT The type of the collection.
     * @param c The c from which to generate `this`.
     *
     * @throws Any exception that may be thrown during the operation.
     */
    Slice(auto && c) requires Iterable<T, decltype(c)>
        : arr_(nullptr), len_(std::distance(std::begin(c), std::end(c))), cap_(len_) {
        allocate();
        size_t i = 0;
        try {
            for (auto && el : std::forward<decltype(c)>(c)) {
                if constexpr (std::is_move_constructible_v<T>) {
                    std::println("Iterable Move");
                    new (arr_ + i) T(std::move(el));
                } else if constexpr (std::is_copy_constructible_v<T>) {
                    std::println("Iterable Copy");
                    new (arr_ + i) T(el);
                } else {
                    static_assert(
                      std::is_constructible_v<T, decltype(el)>,
                      "Element type is neither copy-constructible nor move-constructible"
                    );
                }
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
     * Creates `this` using multiple singular elements.
     * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
     *
     * @tparam Args The types of the elements.
     * @param args The elements to be added to `this`.
     *
     * @throws Any exception that may be thrown during the operation.
     */
    template<typename... Args>
    Slice(Args &&... args) : arr_(nullptr), len_(sizeof...(args)), cap_(len_) {
        allocate();
        size_t i = 0;
        try {
            if constexpr (std::is_move_constructible_v<T>) {
                std::println("Variadic Move");
                ((new (arr_ + i++) T(std::move(args))), ...);
            } else if constexpr (std::is_copy_constructible_v<T>) {
                std::println("Variadic Copy");
                ((new (arr_ + i++) T(args)), ...);
            }
        } catch (...) {
            destroy_elems(i);
            deallocate();
            throw;
        }
    }

    /**
     * @brief Subscript operator.
     *
     * Provides access to the element at the specified index.
     *
     * @param i The index of the element to access.
     * @return A pointer to the element at the specified index.
     *
     * @throws out_of_range if the index is out of bounds.
     */
    T * operator[](size_t i) {
        if (i < 0 || i >= len_) { throw std::out_of_range("Invalid argument"); }
        return &arr_[i];
    }

    /**
     * @brief Slice operator.
     *
     * Provides a sub-slice from the specified start index to the end index.
     *
     * @param i The start index of the sub-slice.
     * @param f The end index of the sub-slice.
     * @return A new `Slice` representing the sub-slice.
     *
     * @throws out_of_range if the indices are out of bounds or invalid.
     */
    Slice<T> operator[](size_t i, size_t f) {
        if (i < 0 || f < 0 || i >= len_ || f >= len_ || f <= i) {
            throw std::out_of_range("Invalid argument");
        }
        return Slice<T>(&arr_[i], f - i);
    }

    /**
     * @brief Converts `this` to a string representation.
     *
     * @return A string representation of `this`.
     */
    std::string toString() {
        std::string s;
        for (size_t i = 0; i < len_; ++i) { s += std::format("{}\n", arr_[i]); }
        return s;
    }

    /**
     * @brief Prints the string representation of `this`.
     */
    void print() {
        std::println("{}", toString());
    }

    /**
     * @brief Destructor.
     *
     * If the elements can be trivially destroyed, it simply deletes the collection and puts `this`
     * in a waiting phase. Otherwise, it calls the destructor for those particular elements.
     */
    ~Slice() noexcept {
        std::println("Destruction");
        destroy_elems(len_);
        deallocate();
    }
};

#endif // SLICE_HXX
