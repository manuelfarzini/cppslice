#include <expected>
#include <print>
#include <string>

template<typename T>
concept NothrowConstructible =
 std::is_nothrow_constructible_v<T> && std::is_nothrow_move_constructible_v<T> &&
 std::is_nothrow_copy_constructible_v<T>;

class NothrowArrayError {
public:
  explicit NothrowArrayError(const std::string & msg) : message(msg) {}
  const std::string & get_message() const { return message; }

private:
  std::string message;
};

struct NothrowType {
  int value;

  NothrowType() noexcept : value(0) {}
  NothrowType(const NothrowType & o) noexcept : value(o.value) {}
  NothrowType(NothrowType &&) noexcept = default;
  NothrowType & operator=(const NothrowType &) noexcept = default;

  static std::expected<NothrowType, NothrowArrayError> make() noexcept {
    if (false) { return std::unexpected(NothrowArrayError("Memory allocation failed")); }
    return NothrowType();
  }
  static std::expected<NothrowType, NothrowArrayError> make(NothrowType o) noexcept {
    if (false) { return std::unexpected(NothrowArrayError("Memory allocation failed")); }
    return NothrowType(o);
  }
};

template<typename T, size_t N>
requires NothrowConstructible<T>
class NothrowArray {
private:
  T data[N];

  NothrowArray(const T (&arr)[N]) noexcept {
    for (size_t i = 0; i < N; ++i) { data[i] = arr[i]; }
  }

public:
  static std::expected<NothrowArray<T, N>, NothrowArrayError> make(const T (&arr)[N]) noexcept {
    T elements[N];

    for (size_t i = 0; i < N; ++i) {
      auto result = T::make(arr[i]);
      if (!result) { return std::unexpected(result.error()); }
      elements[i] = *result;
    }

    return NothrowArray<T, N>(elements);
  }

  T & operator[](size_t index) { return data[index]; }

  const T & operator[](size_t index) const { return data[index]; }

  size_t size() const noexcept { return N; }
};

int main() {
  NothrowType arr[] = {NothrowType(), NothrowType(), NothrowType()};

  auto result = NothrowArray<NothrowType, 3>::make(arr);

  if (result) {
    std::println("NothrowArray created successfully!");
    for (size_t i = 0; i < result->size(); ++i) {
      std::println("Element {} value: {}", i, result->operator[](i).value);
    }
  } else {
    std::println("Error creating NothrowArray: {}", result.error().get_message());
  }

  return 0;
}
