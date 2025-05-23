#include <algorithm>
#include <cstring>
#include <iostream>

class String {
 private:
  size_t sz = 0;
  char* arr = new char[5];
  size_t cap = 4;

  explicit String(size_t count)
      : sz(count), arr(new char[count + 1]), cap(count) {
    arr[count] = '\0';
  }

  size_t find_substr(const String& substring, bool is_rfind) const {
    const char* data_substring = substring.data();
    size_t first_index = sz;
    size_t last_index = sz;
    size_t substring_size = substring.size();
    if (sz < substring_size) {
      return sz;
    }
    for (size_t i = 0; i <= sz - substring_size; ++i) {
      if (memcmp(arr + i, data_substring, sizeof(char) * substring_size) == 0) {
        if (first_index == sz) {
          first_index = i;
          continue;
        }
        last_index = i;
      }
    }
    if (is_rfind) {
      return last_index;
    }
    return first_index;
  }

 public:
  String() = default;

  String(char symbol) : String(1, symbol) {}

  String(const char* str) : String(strlen(str)) {
    std::copy(str, str + sz, arr);
  }

  String(size_t count, char symbol) : String(count) {
    std::fill(arr, arr + count, symbol);
  }

  String(const String& other) : String(other.sz) {
    std::copy(other.arr, other.arr + sz, arr);
  }

  size_t length() const { return sz; }

  size_t size() const { return sz; }

  size_t capacity() const { return cap; }

  void pop_back() {
    --sz;
    arr[sz] = '\0';
  }

  void push_back(char symbol) {
    if (sz == cap) {
      cap *= 2;
      char* copy_string = new char[cap + 1];
      std::copy(arr, arr + sz, copy_string);
      delete[] arr;
      arr = copy_string;
    }
    arr[sz] = symbol;
    ++sz;
    arr[sz] = '\0';
  }

  char& front() { return arr[0]; }

  char& back() { return arr[sz - 1]; }

  const char& front() const { return arr[0]; }

  const char& back() const { return arr[sz - 1]; }

  bool empty() const { return (sz == 0); }

  void clear() {
    if (cap == 0) {
      arr = new char[1];
    }
    sz = 0;
    arr[sz] = '\0';
  }

  String substr(size_t start, size_t count) const {
    count = std::min(count, sz - start);
    String new_string = String(count);
    std::copy(arr + start, arr + start + count, new_string.arr);
    return new_string;
  }

  void shrink_to_fit() {
    char* new_string = new char[sz + 1];
    std::copy(arr, arr + sz, new_string);
    new_string[sz] = '\0';
    delete[] arr;
    arr = new_string;
    cap = sz;
  }

  char* data() { return arr; }

  const char* data() const { return arr; }

  String& operator=(const String& other) {
    if (this == &other) {
      return *this;
    }
    if (cap < other.sz) {
      delete[] arr;
      arr = new char[other.cap + 1];
      cap = other.cap;
    }
    std::copy(other.arr, other.arr + other.sz, arr);
    sz = other.sz;
    arr[sz] = '\0';
    return *this;
  }

  String& operator+=(char symbol) {
    push_back(symbol);
    return *this;
  }

  String& operator+=(const String& other) {
    if (cap < sz + other.sz) {
      cap = std::max(sz + other.sz, 2 * cap);
      char* new_string = new char[cap + 1];
      std::copy(arr, arr + sz, new_string);
      std::copy(other.arr, other.arr + other.sz, new_string + sz);
      sz += other.sz;
      new_string[sz] = '\0';
      delete[] arr;
      arr = new_string;
    } else {
      std::copy(other.arr, other.arr + other.sz, arr + sz);
      sz += other.sz;
      arr[sz] = '\0';
    }
    return *this;
  }

  char& operator[](size_t index) { return arr[index]; }

  const char& operator[](size_t index) const { return arr[index]; }

  size_t find(const String& substring) const {
    return find_substr(substring, false);
  }

  size_t rfind(const String& substring) const {
    return find_substr(substring, true);
  }

  ~String() { delete[] arr; }
};

std::istream& operator>>(std::istream& in, String& other) {
  other.clear();
  char temp = in.get();
  while ((temp < '!') || (temp > '~')) {
    temp = in.get();
  }
  while ((temp >= '!') && (temp <= '~')) {
    other += temp;
    temp = in.get();
  }
  return in;
}

std::ostream& operator<<(std::ostream& out, const String& other) {
  out << other.data();
  return out;
}

String operator+(const String& first, const String& second) {
  String result = first;
  result += second;
  return result;
}

bool operator==(const String& first, const String& second) {
  return (first.size() == second.size()) &&
         !(memcmp(first.data(), second.data(),
                  sizeof(char) * std::min(first.size(), second.size())));
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

bool operator<(const String& first, const String& second) {
  int compare = memcmp(first.data(), second.data(),
                       sizeof(char) * std::min(first.size(), second.size()));
  return compare < 0 || (compare == 0 && (first.size() < second.size()));
}

bool operator>(const String& first, const String& second) {
  return second < first;
}

bool operator>=(const String& first, const String& second) {
  return !(first < second);
}

bool operator<=(const String& first, const String& second) {
  return !(first > second);
}