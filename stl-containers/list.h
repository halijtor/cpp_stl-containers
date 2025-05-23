#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
  };

  struct Node : public BaseNode {
    T value;
    Node(const T& value) : value(value) {}
  };

  BaseNode fake_node_ = {&fake_node_, &fake_node_};
  size_t sz_ = 0;

  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeTraits = typename std::allocator_traits<NodeAlloc>;

  [[no_unique_address]] NodeAlloc alloc_;

  template <bool is_const>
  class base_iterator {
   private:
    BaseNode* node_;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    base_iterator(BaseNode* new_node) : node_(new_node) {}

    template <bool was_const, std::enable_if_t<is_const && !was_const, int> = 0>
    base_iterator(const base_iterator<was_const>& other)
        : node_(other.GetBaseNode()) {}

    reference operator*() const { return static_cast<Node*>(node_)->value; }

    pointer operator->() const { return &static_cast<Node*>(node_)->value; }

    base_iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator temp = *this;
      ++(*this);
      return temp;
    }

    base_iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    base_iterator operator--(int) {
      base_iterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const base_iterator& other) const {
      return node_ == other.node_;
    }

    bool operator!=(const base_iterator& other) const {
      return node_ != other.node_;
    }

    BaseNode* GetBaseNode() const { return node_; }

    Node* GetNode() const { return static_cast<Node*>(node_); }
  };

 public:
  using value_type = T;
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List<T, Alloc>(const Alloc& alloc = Alloc()) : alloc_(alloc) {}

  List<T, Alloc>(size_t init_sz, const Alloc& alloc = Alloc()) : alloc_(alloc) {
    if (init_sz == 0) {
      return;
    }
    BaseNode* prev = &fake_node_;
    try {
      for (size_t i = 0; i < init_sz; ++i) {
        Node* cur = alloc_.allocate(1);
        if constexpr (std::is_default_constructible<T>::value) {
          NodeTraits::construct(alloc_, &cur->value);
        }
        prev->next = cur;
        cur->prev = prev;
        prev = cur;
        ++sz_;
        cur->next = &fake_node_;
        fake_node_.prev = cur;
      }
    } catch (...) {
      size_t temp_sz = sz_;
      for (size_t i = 0; i < temp_sz; ++i) {
        erase(begin());
      }
      throw;
    }
    alloc_ = alloc;
  }

  List<T, Alloc>(size_t init_sz, const T& value, const Alloc& alloc = Alloc())
      : alloc_(alloc) {
    if (init_sz == 0) {
      return;
    }
    BaseNode* prev = &fake_node_;
    try {
      for (size_t i = 0; i < init_sz; ++i) {
        Node* cur = alloc_.allocate(1);
        NodeTraits::construct(alloc_, &cur->value, value);
        prev->next = cur;
        cur->prev = prev;
        prev = cur;
        ++sz_;
        cur->next = &fake_node_;
        fake_node_.prev = cur;
      }
    } catch (...) {
      size_t temp_sz = sz_;
      for (size_t i = 0; i < temp_sz; ++i) {
        erase(begin());
      }
      throw;
    }
  }

  Alloc get_allocator() const { return alloc_; }

  size_t size() const { return sz_; }

  List<T, Alloc>(const List<T, Alloc>& other)
      : alloc_(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.alloc_)) {
    BaseNode* prev = &fake_node_;
    const BaseNode* other_cur = &other.fake_node_;
    try {
      for (size_t i = 0; i < other.sz_; ++i) {
        other_cur = other_cur->next;
        Node* cur = alloc_.allocate(1);
        NodeTraits::construct(alloc_, &cur->value,
                              static_cast<const Node*>(other_cur)->value);
        prev->next = cur;
        cur->prev = prev;
        prev = cur;
        ++sz_;
        cur->next = &fake_node_;
        fake_node_.prev = cur;
      }
    } catch (...) {
      size_t temp_sz = sz_;
      for (size_t i = 0; i < temp_sz; ++i) {
        erase(begin());
      }
      throw;
    }
  }

  List<T, Alloc>& operator=(const List<T, Alloc>& other) {
    if (this != &other) {
      size_t temp_sz = sz_;
      if constexpr (std::allocator_traits<
                        Alloc>::propagate_on_container_copy_assignment::value) {
        size_t temp_sz = sz_;
        for (size_t i = 0; i < temp_sz; ++i) {
          erase(begin());
        }
        alloc_ = other.alloc_;
      }
      const BaseNode* other_cur = other.fake_node_.next;
      BaseNode* curr = fake_node_.next;
      if (sz_ != 0) {
        Node* cur = static_cast<Node*>(fake_node_.next);
        while (other_cur != &other.fake_node_) {
          cur->value = static_cast<const Node*>(other_cur)->value;
          other_cur = other_cur->next;
          if (cur == static_cast<Node*>(fake_node_.prev)) {
            break;
          }
          cur = static_cast<Node*>(cur->next);
        }
        curr = cur;
      }
      try {
        while (sz_ > other.sz_) {
          erase(--end());
        }
        while (other_cur != &other.fake_node_) {
          Node* new_node = alloc_.allocate(1);
          NodeTraits::construct(alloc_, &new_node->value,
                                static_cast<const Node*>(other_cur)->value);
          new_node->prev = fake_node_.prev;
          new_node->next = &fake_node_;
          fake_node_.prev->next = new_node;
          fake_node_.prev = new_node;
          other_cur = other_cur->next;
          ++sz_;
        }
      } catch (...) {
        size_t help = sz_;
        for (size_t i = temp_sz; i < help; ++i) {
          erase(--end());
        }
        throw;
      }
    }
    return *this;
  }

  void insert(const_iterator iter, const T& value) {
    Node* new_node = alloc_.allocate(1);
    NodeTraits::construct(alloc_, &new_node->value, value);
    Node* temp = iter.GetNode();
    new_node->prev = temp->prev;
    new_node->next = temp;
    temp->prev->next = new_node;
    temp->prev = new_node;
    ++sz_;
  }

  void erase(const_iterator iter) {
    Node* temp = iter.GetNode();
    temp->prev->next = temp->next;
    temp->next->prev = temp->prev;
    NodeTraits::destroy(alloc_, &(temp->value));
    alloc_.deallocate(temp, 1);
    --sz_;
  }

  iterator begin() { return iterator(fake_node_.next); }

  const_iterator begin() const { return const_iterator(fake_node_.next); }

  const_iterator cbegin() const { return const_iterator(fake_node_.next); }

  iterator end() { return iterator(&fake_node_); }

  const_iterator end() const {
    return const_iterator(const_cast<BaseNode*>(&fake_node_));
  }

  const_iterator cend() const {
    return const_iterator(const_cast<BaseNode*>(&fake_node_));
  }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  void push_front(const T& value) { insert(cbegin(), value); }

  void push_back(const T& value) { insert(cend(), value); }

  void pop_back() { erase(--end()); }

  void pop_front() { erase(cbegin()); }

  ~List() {
    size_t temp_sz = sz_;
    for (size_t i = 0; i < temp_sz; ++i) {
      erase(begin());
    }
  }
};

template <size_t N>
struct StackStorage {
  alignas(max_align_t) char data[N];
  size_t offset = 0;

  StackStorage() = default;

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  ~StackStorage() = default;

  void* allocate(size_t size, size_t alignment) {
    size_t padding = (alignment - offset % alignment) % alignment;
    size_t adjustedSize = padding + size;
    void* result = data + offset + padding;
    offset += adjustedSize;
    return result;
  }

  void deallocate(void*, size_t) {}
};

template <typename T, size_t N>
class StackAllocator {
 public:
  using value_type = T;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  template <class U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  StackStorage<N>* storage_;

  explicit StackAllocator(StackStorage<N>& storage) : storage_(&storage) {}

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept
      : storage_(other.storage_) {}

  T* allocate(size_t n) {
    return static_cast<T*>(storage_->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T* ptr, size_t n) {
    storage_->deallocate(ptr, n * sizeof(T));
  }

  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& other) {
    storage_ = other.storage_;
    return *this;
  }
};

template <typename T, size_t N, typename U, size_t M>
bool operator==(const StackAllocator<T, N>& first,
                const StackAllocator<U, M>& second) {
  return first.storage_ == second.storage_;
}

template <typename T, size_t N, typename U, size_t M>
bool operator!=(const StackAllocator<T, N>& first,
                const StackAllocator<U, M>& second) {
  return !(first == second);
}