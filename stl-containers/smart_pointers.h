#include <iostream>
#include <memory>
#include <utility>

struct BaseControlBlock {
  size_t shared_count = 0;
  size_t weak_count = 0;

  virtual void* GetPtr() = 0;
  virtual void DeleteObject() = 0;
  virtual void DeleteBlock() = 0;
  virtual ~BaseControlBlock() = default;
};

template <typename T>
class SharedPtr {
 private:

  template <typename U, typename Deleter = std::default_delete<U>,
            typename Alloc = std::allocator<U>>
  struct ControlBlockRegular : BaseControlBlock {
    using BlockAlloc = typename std::allocator_traits<
        Alloc>::template rebind_alloc<ControlBlockRegular<U, Deleter, Alloc>>;
    using BlockAllocTraits = typename std::allocator_traits<BlockAlloc>;

    U* ptr;
    Deleter deleter;
    Alloc alloc;

    ControlBlockRegular(U* ptr, Deleter deleter, Alloc alloc)
        : ptr(ptr), deleter(deleter), alloc(alloc) {}

    void* GetPtr() override { return ptr; }

    void DeleteObject() override { deleter(ptr); }

    void DeleteBlock() override {
      BlockAlloc block_alloc = alloc;
      block_alloc.deallocate(this, 1);
    }

    ~ControlBlockRegular() override = default;
  };

  template <typename U, typename Alloc = std::allocator<U>>
  struct ControlBlockMakeShared : BaseControlBlock {
    using BlockAlloc = typename std::allocator_traits<
        Alloc>::template rebind_alloc<ControlBlockMakeShared>;
    using BlockAllocTraits = std::allocator_traits<BlockAlloc>;
    using ObjectAlloc =
        typename std::allocator_traits<Alloc>::template rebind_alloc<U>;
    using ObjectAllocTraits = std::allocator_traits<ObjectAlloc>;

    U value;
    Alloc alloc;

    template <typename... Args>
    explicit ControlBlockMakeShared(Alloc alloc, Args&&... args)
        : value(std::forward<Args>(args)...), alloc(alloc) {}

    void* GetPtr() override { return &value; }

    void DeleteObject() override {
      ObjectAlloc obj_alloc = alloc;
      ObjectAllocTraits::destroy(obj_alloc, &value);
    }

    void DeleteBlock() override {
      BlockAlloc block_alloc = alloc;
      block_alloc.deallocate(this, 1);
    }

    ~ControlBlockMakeShared() override = default;
  };

  BaseControlBlock* cb_;
  T* ptr_;

  explicit SharedPtr(BaseControlBlock* cb) : cb_(cb) {
    if (cb_ != nullptr) {
      ptr_ = static_cast<T*>(cb->GetPtr());
      ++cb_->shared_count;
    }
  }

 public:
  SharedPtr() : cb_(nullptr), ptr_(nullptr) {}

  template <typename U, typename Deleter = std::default_delete<U>,
            typename Alloc = std::allocator<U>>
  SharedPtr(U* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc())
      : ptr_(static_cast<T*>(ptr)) {
    typename std::allocator_traits<Alloc>::template rebind_alloc<
        ControlBlockRegular<U, Deleter, Alloc>>
        block_alloc = alloc;
    auto cb = block_alloc.allocate(1);
    new (cb) ControlBlockRegular<U, Deleter, Alloc>(ptr, deleter, alloc);
    cb_ = cb;
    ++cb_->shared_count;
  }

  SharedPtr(const SharedPtr& other) : SharedPtr(other.cb_) {
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other)
      : cb_(reinterpret_cast<BaseControlBlock*>(other.cb_)), ptr_(other.ptr_) {
    if (cb_ != nullptr) {
      ++cb_->shared_count;
    }
  }

  SharedPtr(SharedPtr&& other)
      : cb_(reinterpret_cast<BaseControlBlock*>(other.cb_)), ptr_(other.ptr_) {
    other.cb_ = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other)
      : cb_(reinterpret_cast<BaseControlBlock*>(other.cb_)), ptr_(other.ptr_) {
    other.cb_ = nullptr;
    other.ptr_ = nullptr;
  }

  void swap(SharedPtr& other) {
    std::swap(cb_, other.cb_);
    std::swap(ptr_, other.ptr_);
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (this != &other) {
      SharedPtr new_ptr = other;
      swap(new_ptr);
    }
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    if (this != &other) {
      SharedPtr new_ptr = std::move(other);
      swap(new_ptr);
    }
    return *this;
  }

  template <typename Y>
  SharedPtr& operator=(SharedPtr<Y>&& other) {
    SharedPtr new_ptr = std::forward<SharedPtr<Y>>(other);
    swap(new_ptr);
    return *this;
  }

  size_t use_count() const {
    return ((cb_ == nullptr) ? 0 : cb_->shared_count);
  }

  void reset() {
    if (cb_ == nullptr) {
      return;
    }
    if (cb_->shared_count != 0) {
      --(cb_->shared_count);
    }
    if (cb_->shared_count == 0) {
      cb_->DeleteObject();
      if (cb_->weak_count == 0) {
        cb_->DeleteBlock();
      }
    }
    cb_ = nullptr;
    ptr_ = nullptr;
  }

  template <typename Y>
  void reset(Y* ptr) {
    reset();
    *this = SharedPtr(ptr);
  }

  T* get() const { return ptr_; }

  T* operator->() const { return ptr_; }

  T& operator*() const { return *ptr_; }

  ~SharedPtr() { reset(); }

  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(Alloc, Args&&...);
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&&... args) {
  using ControlBlockType =
      typename SharedPtr<T>::template ControlBlockMakeShared<T, Alloc>;
  using BlockAlloc = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockType>;
  using BlockAllocTraits = std::allocator_traits<BlockAlloc>;
  BlockAlloc block_alloc(alloc);
  ControlBlockType* cb = block_alloc.allocate(1);
  BlockAllocTraits::construct(block_alloc, cb, alloc,
                              std::forward<Args>(args)...);
  auto block = static_cast<BaseControlBlock*>(cb);
  return SharedPtr<T>(block);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T, std::allocator<T>>(std::allocator<T>(),
                                              std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 private:
  BaseControlBlock* cb_;

 public:
  WeakPtr() : cb_(nullptr) {}

  template <typename U>
  WeakPtr(SharedPtr<U>& ptr)
      : cb_(reinterpret_cast<BaseControlBlock*>(ptr.cb_)) {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  WeakPtr(const WeakPtr& other) : cb_(other.cb_) {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other)
      : cb_(reinterpret_cast<BaseControlBlock*>(other.cb_)) {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  void swap(WeakPtr& other) { std::swap(cb_, other.cb_); }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this != &other) {
      WeakPtr new_ptr = other;
      swap(new_ptr);
    }
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    if (this != &other) {
      WeakPtr new_ptr = std::move(other);
      swap(new_ptr);
    }
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=(WeakPtr<Y>&& other) {
    if (this != &other) {
      WeakPtr new_ptr = std::forward<WeakPtr<Y>>(other);
      swap(new_ptr);
    }
    return *this;
  }

  size_t use_count() { return cb_->shared_count; }

  bool expired() const { return cb_->shared_count == 0; }

  SharedPtr<T> lock() const {
    return ((expired()) ? SharedPtr<T>() : SharedPtr<T>(cb_));
  }

  ~WeakPtr() {
    if (cb_ == nullptr) {
      return;
    }
    --(cb_->weak_count);
    if (cb_->weak_count == 0 && cb_->shared_count == 0) {
      cb_->DeleteBlock();
    }
    cb_ = nullptr;
  }

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;
};

template <typename T>
class EnableSharedFromThis {
 private:
  WeakPtr<T> ptr_;

 public:
  SharedPtr<T> shared_from_this() const { return ptr_.lock(); }
};