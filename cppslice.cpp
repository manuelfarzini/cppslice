#include <type_traits>
#include <vector>

template<typename T>
class Slice {
public:
private:
    /** The collection. */
    T * _arr;

    /** The number of elements in This. */
    size_t _size;

    /** The maximum capacity of This. */
    size_t _capacity;

    /*-
     * The abstraction function AF is
     *      AF(_arr) = [a_1, a_2, …, a__size]
     *
     * The representation invariant RI is
     *      - 0 ≤ _size ≤ _capacity
     *      - typeof(_arr[i]) = typeof(_arr[j])  FORALL  i,j = 0, …, _size
     */

    /**
     * Utility function to allocate the memory for the slice.
     *
     * <p>
     * It allocates memory of the specified size and sets This view on that chunk of data.
     *
     * @param size the size to allocate.
     */
    void allocate(size_t size);

    /**
     *  TODO:
     */
    void deallocate();

    /**
     *  TODO:
     */
    void destroy_elems(size_t count);

    static_assert(
      std::is_trivially_destructible_v<T> || std::is_destructible_v<T>,
      "Type T must provide a destructor if it is not trivially destructible"
    );

public:
    /**
     * Default constructor. Creates an empty slice.
     */
    Slice();

    /**
     * Size constructor.
     *
     * <p>
     * Creates a slice of the given size. The elements of the collection are uninitialized.
     *
     * @param size the initial size of the slice.
     */
    Slice(size_t size);

    /**
     * Array constructor.
     *
     * <p>
     * Creates a slice taking an existing collection.
     * Specifically, it creates a slice focusing This view over an existing raw C-style array.
     *
     * @param brr the array to view.
     * @param size the size of brr
     */
    Slice(T * brr, size_t slice);

    /**
     * Templated constructor.
     *
     * <p>
     * Creates a slice taking an existing collection of elements.
     * Specifically, it creates a slice using any iterable C++ collection.
     *
     * <p>
     * The collection can be either copied or moved. In the second case, you should not reuse it.
     *
     * <p>
     * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
     *
     * @param container the container from which to generate the slice.
     * @throws any exception if thrown.
     */
    template<typename Container>
    requires std::is_same_v<T, typename std::remove_reference_t<Container>::value_type>
    Slice(Container && container);

    /**
     * Variadic constructor.
     *
     * <p>
     * Creates a slice using multiple singular elements.
     *
     * <p>
     * If an exception is thrown, it triggers a cleanup routine and propagates the exception.
     *
     * @throws any exception if thrown.
     */
    template<typename... Args>
    requires(std::is_constructible_v<T, Args &&> && ...)
    Slice(Args &&... args);

    /**
     * Destructor.
     *
     * <p>
     * If the elements can be trivially destroyed, it simply deletes the collection and puts This
     * in a waiting phase. Otherwise, it calls the destructor for those particular elements.
     */
    ~Slice() noexcept;
};

template<typename T>
Slice<T>::Slice() : _arr(nullptr), _size(0), _capacity(0) {}

template<typename T>
Slice<T>::Slice(size_t cap) : _arr(nullptr), _size(0), _capacity(cap) {
    allocate(cap);
}

template<typename T>
Slice<T>::Slice(T * brr, size_t size) : _arr(brr), _size(size), _capacity(size) {
    if (brr == nullptr && size > 0) {
        throw std::invalid_argument(
          "Error: a slice cannot be nullptr if the size is greater than zero."
        );
    }
}

template<typename T>
template<typename Container>
requires std::is_same_v<T, typename std::remove_reference_t<Container>::value_type>
Slice<T>::Slice(Container && container)
    : _arr(nullptr),
      _size(std::distance(std::begin(container), std::end(container))),
      _capacity(_size) {
    allocate(_capacity);
    size_t i = 0;
    try {
        for (auto && elem : std::forward<Container>(container)) {
            _arr[i++] = std::forward<decltype(elem)>(elem);
        }
    } catch (...) {
        destroy_elems(i);
        deallocate();
        throw;
    }
}

template<typename T>
template<typename... Args>
requires(std::is_constructible_v<T, Args &&> && ...)
Slice<T>::Slice(Args &&... args) : _arr(nullptr), _size(sizeof...(args)), _capacity(_size) {
    allocate(_capacity);
    size_t i = 0;
    try {
        ((_arr[i++] = T(std::forward<Args>(args))), ...);
    } catch (...) {
        destroy_elems(i);
        deallocate();
        throw;
    }
}

template<typename T>
Slice<T>::~Slice() noexcept {
    destroy_elems(_size);
    deallocate();
}

template<typename T>
void Slice<T>::destroy_elems(size_t count) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (size_t i = 0; i < count; ++i) {
            _arr[i].~T();
        }
    }
}

template<typename T>
void Slice<T>::allocate(size_t cap) {
    _arr = static_cast<T *>(::operator new[](cap * sizeof(T)));
}

template<typename T>
void Slice<T>::deallocate() {
    if (_arr) {
        ::operator delete[](_arr);
        _arr = nullptr;
    }
}

int main() {
    std::vector<int> v1 = {2, 3, 4, 5, 6};
    Slice<int> s1(v1);
    Slice<int> s2(std::move(v1));
    Slice<int> s3(22, 23, 24, 25, 26);
}
