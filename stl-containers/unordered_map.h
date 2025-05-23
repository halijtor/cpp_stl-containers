#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
  };

  struct Node : public BaseNode {
    T value;

    Node() = default;
    Node(const T& value) : value(value) {}
  };

  BaseNode* fake_node_;
  size_t sz_ = 0;

  using BaseNodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<BaseNode>;
  using BaseNodeTraits = typename std::allocator_traits<Alloc>::template rebind_traits<BaseNode>;
  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeTraits = typename std::allocator_traits<NodeAlloc>;

  [[no_unique_address]] NodeAlloc alloc_;

  void construct_fake_node() {
    BaseNodeAlloc allocator = alloc_;
    fake_node_ = BaseNodeTraits::allocate(allocator, 1);
    fake_node_->prev = fake_node_;
    fake_node_->next = fake_node_;
  }

  template <bool is_const>
  class base_iterator {
   private:
    BaseNode* node_ = nullptr;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    base_iterator() = default;

    base_iterator(BaseNode* new_node) : node_(new_node) {}

    base_iterator(const base_iterator<false>& other)
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

    base_iterator& operator=(const base_iterator<false>& other) {
      node_ = other.node_;
      return *this;
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

  void swap(List<T, Alloc>& other) {
    std::swap(fake_node_, other.fake_node_);
    std::swap(sz_, other.sz_);
    if (NodeTraits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  explicit List(const Alloc& alloc = Alloc())
      : alloc_(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                alloc)) {
    construct_fake_node();
  }

  explicit List(size_t init_sz, const Alloc& alloc = Alloc())
      : alloc_(alloc) {
    construct_fake_node();
    BaseNode* prev = fake_node_;
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
        cur->next = fake_node_;
        fake_node_->prev = cur;
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

  List(size_t init_sz, const T& value, const Alloc& alloc = Alloc())
      : alloc_(alloc) {
    construct_fake_node();
    if (init_sz == 0) {
      return;
    }
    BaseNode* prev = fake_node_;
    try {
      for (size_t i = 0; i < init_sz; ++i) {
        Node* cur = alloc_.allocate(1);
        NodeTraits::construct(alloc_, &cur->value, value);
        prev->next = cur;
        cur->prev = prev;
        prev = cur;
        ++sz_;
        cur->next = fake_node_;
        fake_node_->prev = cur;
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

  List(const List<T, Alloc>& other)
      : alloc_(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.alloc_)) {
    construct_fake_node();
    BaseNode* prev = fake_node_;
    const BaseNode* other_cur = other.fake_node_;
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
        cur->next = fake_node_;
        fake_node_->prev = cur;
      }
    } catch (...) {
      size_t temp_sz = sz_;
      for (size_t i = 0; i < temp_sz; ++i) {
        erase(begin());
      }
      throw;
    }
  }

  template <typename... Args>
  void emplace(const_iterator iter, Args&&... args) {
    Node* node = iter.GetNode();
    Node* new_node = alloc_.allocate(1);
    try {
      NodeTraits::construct(alloc_, &(new_node->value),
                            std::forward<Args>(args)...);
    } catch (...) {
      alloc_.deallocate(new_node, 1);
      throw;
    }
    new_node->next = node;
    new_node->prev = node->prev;
    node->prev->next = new_node;
    node->prev = new_node;
    ++sz_;
  }

  List(List<T, Alloc>&& other) : List() { swap(other); }

  List& operator=(const List<T, Alloc>& other) {
    if (this != &other) {
      size_t temp_sz = sz_;
      if (std::allocator_traits<
              Alloc>::propagate_on_container_copy_assignment::value) {
        alloc_ = other.alloc_;
      }
      construct_fake_node();
      const BaseNode* other_cur = other.fake_node_->next;
      if (sz_ != 0) {
        Node* cur = static_cast<Node*>(fake_node_->next);
        while (other_cur != other.fake_node_) {
          cur->value = static_cast<const Node*>(other_cur)->value;
          other_cur = other_cur->next;
          if (cur == static_cast<Node*>(fake_node_->prev)) {
            break;
          }
          cur = static_cast<Node*>(cur->next);
        }
      }
      try {
        while (sz_ > other.sz_) {
          erase(--end());
        }
        while (other_cur != other.fake_node_) {
          Node* new_node = alloc_.allocate(1);
          NodeTraits::construct(alloc_, &new_node->value,
                                static_cast<const Node*>(other_cur)->value);
          new_node->prev = fake_node_->prev;
          new_node->next = fake_node_;
          fake_node_->prev->next = new_node;
          fake_node_->prev = new_node;
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

  List& operator=(List<T, Alloc>&& other) {
    if (this != &other) {
      while (sz_ > 0) {
        erase(cbegin());
      }
      if (NodeTraits::propagate_on_container_move_assignment::value) {
        alloc_ = std::move(other.alloc_);
        swap(other);
      } else if (alloc_ == other.alloc_) {
        swap(other);
      } else {
        /*for (auto it = other.begin(); it != other.end(); ++it) {
          emplace(cend(), std::move(*it));
        }
        while (other.sz_ > 0) {
          other.erase(other.cbegin());
        }*/
        swap(other);
      }
    }
    return *this;
  }

  void insert(const_iterator iter, const T& value) { emplace(iter, value); }

  void erase(const_iterator iter) {
    Node* temp = iter.GetNode();
    if (temp == nullptr) {
      return;
    }
    temp->prev->next = temp->next;
    temp->next->prev = temp->prev;
    NodeTraits::destroy(alloc_, &(temp->value));
    alloc_.deallocate(temp, 1);
    --sz_;
  }

  iterator begin() { return iterator(fake_node_->next); }

  const_iterator begin() const { return const_iterator(fake_node_->next); }

  const_iterator cbegin() const { return const_iterator(fake_node_->next); }

  iterator end() { return iterator(fake_node_); }

  const_iterator end() const {
    return const_iterator(fake_node_);
  }

  const_iterator cend() const {
    return const_iterator(fake_node_);
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
      erase(cbegin());
    }
    BaseNodeAlloc allocator = alloc_;
    BaseNodeTraits::deallocate(allocator, fake_node_, 1);
  }

  template <typename Key, typename Value, typename Hash, typename Equal,
            typename Allocator>
  friend class UnorderedMap;
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
  using NodeType = std::pair<const Key, Value>;
  using AllocTraits = std::allocator_traits<Allocator>;
  using ListType =
      List<NodeType, typename AllocTraits::template rebind_alloc<NodeType>>;
  using ListIterator = typename ListType::iterator;
  using ListConstIterator = typename ListType::const_iterator;
  using ListIteratorAlloc =
      typename AllocTraits::template rebind_alloc<ListIterator>;
  [[no_unique_address]] Hash hash_;
  [[no_unique_address]] Equal equal_;
  [[no_unique_address]] Allocator alloc_;
  std::vector<ListIterator, ListIteratorAlloc> hash_table_;
  ListType list_;
  float max_load_factor_ = 0.9f;
  static const size_t max_search_dist_ = 16;
  static const size_t start_bucket_count_ = 16;

  void rehash() {
    bool tmp = true;
    size_t count = hash_table_.size();
    hash_table_.assign(count, nullptr);
    for (auto iter = list_.begin(); iter != list_.end(); ++iter) {
      size_t hash_id = hash_(iter->first) % count;
      size_t end = std::min(count, hash_id + max_search_dist_);
      for (size_t i = hash_id; i < end; ++i) {
        if (hash_table_[i % count].GetNode() == nullptr) {
          hash_table_[i % count] = iter;
          break;
        } else if (i == end - 1) {
          tmp = false;
        }
      }
      if (!tmp) {
        break;
      }
    }
    if (!tmp) {
      hash_table_.assign(count, nullptr);
      for (auto iter = list_.begin(); iter != list_.end(); ++iter) {
        size_t hash_id = hash_(iter->first) % count;
        if (hash_table_[hash_id].GetNode() == nullptr) {
          hash_table_[hash_id] = iter;
        } else {
          size_t end = std::min(count, hash_id + max_search_dist_);
          size_t i;
          for (i = hash_id + 1;
               i < end && hash_table_[i % count].GetNode() != nullptr; ++i) {
          }
          hash_table_[i % count] = iter;
        }
      }
    }
  }

 public:
  using iterator = typename ListType::template base_iterator<false>;
  using const_iterator = typename ListType::template base_iterator<true>;

  void swap(UnorderedMap& other) {
    std::swap(hash_, other.hash_);
    std::swap(equal_, other.equal_);
    std::swap(hash_table_, other.hash_table_);
    std::swap(list_, other.list_);
    std::swap(max_load_factor_, other.max_load_factor_);
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  void reserve(size_t n) {
    if (n > hash_table_.size()) {
      hash_table_.resize(n);
      rehash();
    }
  }

  float load_factor() const {
    return static_cast<float>(list_.size()) / hash_table_.size();
  }

  float max_load_factor() const { return max_load_factor_; }

  void max_load_factor(float ml) { max_load_factor_ = ml; }

  UnorderedMap() : alloc_(Allocator()), hash_table_(alloc_), list_(alloc_) {
    reserve(start_bucket_count_);
  }

  UnorderedMap(const UnorderedMap& other)
      : alloc_(
            AllocTraits::select_on_container_copy_construction(other.alloc_)),
        hash_table_(alloc_),
        list_(other.list_) {
    reserve(other.hash_table_.size());
  }

  UnorderedMap(UnorderedMap&& other)
      : alloc_(std::move(other.alloc_)),
        hash_table_(std::move(other.hash_table_)),
        list_(std::move(other.list_)) {
    reserve(other.hash_table_.size());
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this != &other) {
      UnorderedMap temp(other);
      swap(temp);
    }
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) {
    if (this != &other) {
      hash_table_ = std::move(other.hash_table_);
      list_ = std::move(other.list_);
      hash_ = std::move(other.hash_);
      equal_ = std::move(other.equal_);
      if (AllocTraits::propagate_on_container_move_assignment::value) {
        alloc_ = std::move(other.alloc_);
      }
    }
    return *this;
  }

  size_t size() const { return list_.size(); }

  iterator begin() { return list_.begin(); }

  const_iterator begin() const { return list_.cbegin(); }

  const_iterator cbegin() const { return list_.cbegin(); }

  iterator end() { return list_.end(); }

  const_iterator end() const { return list_.cend(); }

  const_iterator cend() const { return list_.cend(); }

  iterator find(const Key& key) {
    size_t hash_id = hash_(key) % hash_table_.size();
    for (size_t i = 0; i < max_search_dist_; ++i) {
      size_t index = (hash_id + i) % hash_table_.size();
      if (hash_table_[index].GetNode() == nullptr) {
        return list_.end();
      }
      if (hash_table_[index].GetNode() == list_.fake_node_) {
        continue;
      }
      if (equal_(key, hash_table_[index]->first)) {
        return hash_table_[index];
      }
    }
    return list_.end();
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    list_.emplace(list_.end(), std::forward<Args>(args)...);
    iterator new_element = --list_.end();
    size_t hash_index = hash_(new_element->first) % hash_table_.size();
    for (size_t probe = 0; probe < max_search_dist_; ++probe) {
      size_t index = (hash_index + probe) % hash_table_.size();
      if (hash_table_[index].GetNode() == nullptr) {
        hash_table_[index] = new_element;
        if (load_factor() > max_load_factor_) {
          reserve(hash_table_.size() * 2);
        }
        return {new_element, true};
      }
      if (hash_table_[index].GetNode() == list_.fake_node_) {
        continue;
      }
      if (equal_(new_element->first, hash_table_[index]->first)) {
        list_.pop_back();
        return {hash_table_[index], false};
      }
    }
    reserve(hash_table_.size() * 2);
    list_.erase(--list_.cend());
    return emplace(std::forward<Args>(args)...);
  }

  void insert(const NodeType& node) { emplace(node); }

  void insert(NodeType&& node) {
    emplace(std::move(const_cast<Key&>(node.first)), std::move(node.second));
  }

  template <typename InputIterator>
  void insert(const InputIterator& first, const InputIterator& second) {
    for (auto iter = first; iter != second; ++iter) {
      emplace(*iter);
    }
  }

  void erase(iterator iter) {
    size_t hash_id = hash_(iter->first) % hash_table_.size();
    for (size_t i = 0; i < max_search_dist_; ++i) {
      if (hash_table_[(hash_id + i) % hash_table_.size()] == iter) {
        hash_table_[(hash_id + i) % hash_table_.size()] = iterator();
        break;
      }
    }
    list_.erase(iter);
  }

  void erase(const Key& key) {
    iterator iter = find(key);
    if (iter != list_.end()) {
      erase(iter);
    }
  }

  void erase(iterator first, iterator second) {
    while (first != second) {
      iterator next = std::next(first);
      erase(first);
      first = next;
    }
  }

  Value& operator[](const Key& key) {
    iterator it = find(key);
    if (it == list_.end()) {
      return emplace(key, Value()).first->second;
    }
    return it->second;
  }

  Value& operator[](Key&& key) {
    auto it = find(key);
    if (it == list_.end()) {
      return emplace(std::move(key), Value()).first->second;
    }
    return it->second;
  }

  Value& at(const Key& key) {
    auto it = find(key);
    if (it != list_.end()) {
      return it->second;
    }
    throw std::out_of_range("");
  }

  const Value& at(const Key& key) const {
    const_iterator it = find(key);
    if (it != list_.end()) {
      return it->second;
    }
    throw std::out_of_range("");
  }
};