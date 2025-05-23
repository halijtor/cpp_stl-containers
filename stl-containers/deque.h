#include <stddef.h>

#include <iostream>
#include <iterator>
#include <type_traits>

template <typename T>
class Deque {
 private:
  T** outer_array_ = nullptr;
  size_t sz_ = 0;
  size_t cap_ = 0;
  static const size_t block_sz_ = 16;

  struct Position {
    int64_t row = 0;
    int64_t column = 0;

    Position() = default;

    Position(ssize_t x, ssize_t y) : row(x), column(y) {}

    Position& operator++() {
      ++column;
      if (column == block_sz_) {
        column = 0;
        ++row;
      }
      return *this;
    }

    Position& operator--() {
      --column;
      if (column < 0) {
        column = block_sz_ - 1;
        --row;
      }
      return *this;
    }
  };

  Position begin_;
  Position end_;

  void resize(size_t cap, bool front) {
    size_t help = 0;
    size_t newcap = (cap == 0) ? 4 : 2 * cap;
    T** newarr;
    try {
      newarr = new T*[newcap];
      for (size_t i = 0; i < newcap; ++i) {
        newarr[i] = reinterpret_cast<T*>(new char[block_sz_ * sizeof(T)]);
        ++help;
      }
    } catch (...) {
      for (size_t i = 0; i < help; ++i) {
        delete[] reinterpret_cast<char*>(newarr[i]);
      }
      delete[] newarr;
      throw;
    }
    if (front) {
      for (size_t i = 0; i < cap_; ++i) {
        newarr[i + cap_] = outer_array_[i];
      }
      begin_.row += cap_;
      end_.row += cap_;
    } else {
      for (size_t i = 0; i < cap_; ++i) {
        newarr[i] = outer_array_[i];
      }
    }
    delete[] outer_array_;
    outer_array_ = newarr;
    cap_ = newcap;
  }

 public:
  Deque<T>() = default;

  Deque<T>(const Deque<T>& other) {
    try {
      outer_array_ = new T*[other.cap_];
      for (size_t i = 0; i < other.cap_; ++i) {
        outer_array_[i] = reinterpret_cast<T*>(new char[block_sz_ * sizeof(T)]);
        ++cap_;
      }
    } catch (...) {
      for (size_t i = 0; i < cap_; ++i) {
        delete[] reinterpret_cast<char*>(outer_array_[i]);
      }
      delete[] outer_array_;
      cap_ = 0;
      throw;
    }
    begin_ = other.begin_;
    end_ = other.end_;
    try {
      for (size_t i = 0; i < other.sz_; ++i) {
        new (&((*this)[i])) T(other[i]);
        ++sz_;
      }
    } catch (...) {
      for (size_t i = 0; i < sz_; ++i) {
        (&((*this)[i]))->~T();
      }
      for (size_t i = 0; i < cap_; ++i) {
        delete[] reinterpret_cast<char*>(outer_array_[i]);
      }
      delete[] outer_array_;
      sz_ = 0;
      cap_ = 0;
      begin_ = {0, 0};
      end_ = {0, 0};
      throw;
    }
  }

  T& operator[](size_t index) {
    int x = begin_.row + (begin_.column + index) / block_sz_;
    int y = (begin_.column + index) % block_sz_;
    return outer_array_[x][y];
  }

  const T& operator[](size_t index) const {
    int x = begin_.row + (begin_.column + index) / block_sz_;
    int y = (begin_.column + index) % block_sz_;
    return outer_array_[x][y];
  }

  T& at(size_t index) {
    if (index >= sz_) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  const T& at(size_t index) const {
    if (index >= sz_) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  Deque<T>(size_t init_sz) {
    cap_ = 2 * (init_sz / block_sz_ + 1);
    size_t help = 0;
    try {
      outer_array_ = new T*[cap_];
      for (size_t i = 0; i < cap_; ++i) {
        outer_array_[i] = reinterpret_cast<T*>(new char[block_sz_ * sizeof(T)]);
        ++help;
      }
    } catch (...) {
      for (size_t i = 0; i < help; ++i) {
        delete[] reinterpret_cast<char*>(outer_array_[i]);
      }
      delete[] outer_array_;
      cap_ = 0;
      throw;
    }
    begin_ = {0, 0};
    end_ = begin_;
    for (size_t i = 0; i + 1 < init_sz; ++i) {
      ++end_;
    }
    if constexpr (std::is_default_constructible<T>::value) {
      try {
        for (size_t i = 0; i < init_sz; ++i) {
          new (&((*this)[i])) T();
          ++sz_;
        }
      } catch (...) {
        for (size_t i = 0; i < sz_; ++i) {
          (&((*this)[i]))->~T();
        }
        for (size_t i = 0; i < cap_; ++i) {
          delete[] reinterpret_cast<char*>(outer_array_[i]);
        }
        delete[] outer_array_;
        sz_ = 0;
        cap_ = 0;
        begin_ = {0, 0};
        end_ = {0, 0};
        throw;
      }
    }
  }

  Deque<T>(size_t init_sz, const T& value) {
    cap_ = 2 * (init_sz / block_sz_ + 1);
    size_t help = 0;
    try {
      outer_array_ = new T*[cap_];
      for (size_t i = 0; i < cap_; ++i) {
        outer_array_[i] = reinterpret_cast<T*>(new char[block_sz_ * sizeof(T)]);
        ++help;
      }
    } catch (...) {
      for (size_t i = 0; i < help; ++i) {
        delete[] reinterpret_cast<char*>(outer_array_[i]);
      }
      delete[] outer_array_;
      cap_ = 0;
      throw;
    }
    begin_ = {0, 0};
    end_ = begin_;
    for (size_t i = 0; i + 1 < init_sz; ++i) {
      ++end_;
    }
    try {
      for (size_t i = 0; i < init_sz; ++i) {
        new (&((*this)[i])) T(value);
        ++sz_;
      }
    } catch (...) {
      for (size_t i = 0; i < sz_; ++i) {
        (&((*this)[i]))->~T();
      }
      for (size_t i = 0; i < cap_; ++i) {
        delete[] reinterpret_cast<char*>(outer_array_[i]);
      }
      delete[] outer_array_;
      sz_ = 0;
      cap_ = 0;
      begin_ = {0, 0};
      end_ = {0, 0};
      throw;
    }
  }

  void swap(Deque<T>& other) noexcept {
    std::swap(cap_, other.cap_);
    std::swap(sz_, other.sz_);
    std::swap(outer_array_, other.outer_array_);
    std::swap(begin_, other.begin_);
    std::swap(end_, other.end_);
  }

  Deque<T>& operator=(const Deque<T>& other) {
    if (this != &other) {
      Deque<T> temp(other);
      swap(temp);
    }
    return *this;
  }

  size_t size() const { return sz_; }

  void push_back(const T& value) {
    Position rem = end_;
    ++end_;
    if (end_.row >= static_cast<long>(cap_)) {
      --end_;
      if (cap_ == 0) {
        --end_;
      }
      resize(cap_, false);
      ++end_;
    }
    try {
      new (&((*this)[sz_])) T(value);
      ++sz_;
    } catch (...) {
      end_ = rem;
      throw;
    }
  }

  void push_front(const T& value) {
    Position rem = begin_;
    --begin_;
    if (begin_.row < 0) {
      ++begin_;
      if (cap_ == 0) {
        ++begin_;
      }
      resize(cap_, true);
      --begin_;
    }
    try {
      new (&((*this)[0])) T(value);
      ++sz_;
    } catch (...) {
      begin_ = rem;
      throw;
    }
  }

  void pop_front() {
    (*this)[0].~T();
    ++begin_;
    --sz_;
  }

  void pop_back() {
    --sz_;
    (*this)[sz_].~T();
    --end_;
  }

  ~Deque() {
    for (size_t i = 0; i < sz_; ++i) {
      (*this)[i].~T();
    }
    for (size_t i = 0; i < cap_; ++i) {
      delete[] reinterpret_cast<char*>(outer_array_[i]);
    }
    delete[] outer_array_;
  }

  template <bool is_const>
  class base_iterator {
   private:
    typedef typename std::conditional<is_const, const T**, T**>::type Storage;
    typedef typename std::conditional<is_const, const T*, T*>::type Pointer;
    typedef typename std::conditional<is_const, const T&, T&>::type Reference;

    Storage bucket_ptr_;
    Pointer elem_ptr_;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    base_iterator(T** new_bucket, T* new_elem)
        : bucket_ptr_(const_cast<Storage>(new_bucket)),
          elem_ptr_(const_cast<Pointer>(new_elem)) {}

    template <bool was_const, std::enable_if_t<is_const && !was_const, int> = 0>
    base_iterator(const base_iterator<was_const>& other)
        : bucket_ptr_(other.bucket_ptr_), elem_ptr_(other.elem_ptr_) {}

    Reference operator*() const { return *elem_ptr_; }

    Pointer operator->() const { return elem_ptr_; }

    base_iterator& operator+=(int shift) {
      if (shift == 0 || bucket_ptr_ == nullptr) {
        return *this;
      }
      if (shift > 0) {
        int rem = elem_ptr_ - *bucket_ptr_;
        bucket_ptr_ += (shift + rem) / block_sz_;
        elem_ptr_ = *bucket_ptr_ + (shift + rem) % block_sz_;
      } else {
        shift = -shift;
        if (shift > elem_ptr_ - *bucket_ptr_) {
          shift -= (elem_ptr_ - *bucket_ptr_) + 1;
          bucket_ptr_ -= shift / block_sz_ + 1;
          elem_ptr_ = *bucket_ptr_ + (block_sz_ - shift % block_sz_ - 1);
        } else {
          elem_ptr_ -= shift;
        }
      }
      return *this;
    }

    base_iterator& operator-=(int shift) {
      shift = -shift;
      *this += shift;
      return *this;
    }

    base_iterator& operator++() {
      *this += 1;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator temp = *this;
      ++temp;
      return temp;
    }

    base_iterator& operator--() {
      *this -= 1;
      return *this;
    }

    base_iterator operator--(int) {
      base_iterator temp = *this;
      --temp;
      return temp;
    }

    base_iterator operator+(difference_type shift) const {
      base_iterator temp = *this;
      temp += shift;
      return temp;
    }

    base_iterator operator-(difference_type shift) const {
      base_iterator temp = *this;
      temp -= shift;
      return temp;
    }

    difference_type operator-(const base_iterator& other) const {
      if (bucket_ptr_ == nullptr) {
        return 0;
      }
      return (bucket_ptr_ - other.bucket_ptr_) * block_sz_ +
             (elem_ptr_ - *bucket_ptr_) -
             (other.elem_ptr_ - *other.bucket_ptr_);
    }

    bool operator==(const base_iterator& other) const {
      return *this - other == 0;
    }

    std::weak_ordering operator<=>(const base_iterator& other) const {
      return (*this - other) <=> 0;
    }
  };

  typedef base_iterator<false> iterator;
  typedef base_iterator<true> const_iterator;

  iterator begin() {
    if (sz_ == 0) {
      return iterator(nullptr, nullptr);
    }
    return iterator(outer_array_ + begin_.row,
                    outer_array_[begin_.row] + begin_.column);
  }

  const_iterator begin() const {
    if (sz_ == 0) {
      return const_iterator(nullptr, nullptr);
    }
    return const_iterator(outer_array_ + begin_.row,
                          outer_array_[begin_.row] + begin_.column);
  }

  iterator end() {
    if (sz_ == 0) {
      return begin();
    }
    if (end_.column == block_sz_ - 1) {
      return iterator(outer_array_ + end_.row + 1, outer_array_[end_.row + 1]);
    }
    return iterator(outer_array_ + end_.row,
                    outer_array_[end_.row] + end_.column + 1);
  }

  const_iterator end() const {
    if (sz_ == 0) {
      return begin();
    }
    if (end_.column == block_sz_ - 1) {
      return const_iterator(outer_array_ + end_.row + 1,
                            outer_array_[end_.row + 1]);
    }
    return const_iterator(outer_array_ + end_.row,
                          outer_array_[end_.row] + end_.column + 1);
  }

  const_iterator cbegin() const {
    if (sz_ == 0) {
      return const_iterator(nullptr, nullptr);
    }
    return const_iterator(outer_array_ + begin_.row,
                          outer_array_[begin_.row] + begin_.column);
  }

  const_iterator cend() const {
    if (sz_ == 0) {
      return cbegin();
    }
    if (end_.column == block_sz_ - 1) {
      return const_iterator(outer_array_ + end_.row + 1,
                            outer_array_[end_.row + 1]);
    }
    return const_iterator(outer_array_ + end_.row,
                          outer_array_[end_.row] + end_.column + 1);
  }

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  reverse_iterator rend() {
    if (sz_ == 0) {
      return reverse_iterator(iterator(nullptr, nullptr));
    }
    return reverse_iterator(iterator(outer_array_ + begin_.row,
                                     outer_array_[begin_.row] + begin_.column));
  }

  const_reverse_iterator rend() const {
    if (sz_ == 0) {
      return const_reverse_iterator(iterator(nullptr, nullptr));
    }
    return const_reverse_iterator(const_iterator(
        outer_array_ + begin_.row, outer_array_[begin_.row] + begin_.column));
  }

  reverse_iterator rbegin() {
    if (sz_ == 0) {
      return rend();
    }
    if (end_.column == block_sz_ - 1) {
      return reverse_iterator(
          iterator(outer_array_ + end_.row + 1, outer_array_[end_.row + 1]));
    }
    return reverse_iterator(iterator(outer_array_ + end_.row,
                                     outer_array_[end_.row] + end_.column + 1));
  }

  const_reverse_iterator rbegin() const {
    if (sz_ == 0) {
      return rend();
    }
    if (end_.column == block_sz_ - 1) {
      return const_reverse_iterator(
          iterator(outer_array_ + end_.row + 1, outer_array_[end_.row + 1]));
    }
    return const_reverse_iterator(iterator(
        outer_array_ + end_.row, outer_array_[end_.row] + end_.column + 1));
  }

  const_reverse_iterator crend() const {
    if (sz_ == 0) {
      return const_reverse_iterator(iterator(nullptr, nullptr));
    }
    return const_reverse_iterator(const_iterator(
        outer_array_ + begin_.row, outer_array_[begin_.row] + begin_.column));
  }

  const_reverse_iterator crbegin() const {
    if (sz_ == 0) {
      return crend();
    }
    if (end_.column == block_sz_ - 1) {
      return const_reverse_iterator(
          iterator(outer_array_ + end_.row + 1, outer_array_[end_.row + 1]));
    }
    return const_reverse_iterator(iterator(
        outer_array_ + end_.row, outer_array_[end_.row] + end_.column + 1));
  }

  void insert(iterator it, const T& val) {
    T temp = val;
    for (iterator iter = it; iter != end(); ++iter) {
      std::swap(*iter, temp);
    }
    push_back(temp);
  }

  void erase(iterator it) {
    for (iterator iter = it; iter + 1 != end(); ++iter) {
      std::swap(*iter, *(iter + 1));
    }
    pop_back();
  }
};