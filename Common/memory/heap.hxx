#pragma once

#include "core/sync.hxx"
#include "types.h"
#include <EASTL/type_traits.h>

namespace kuribo::mem {

struct Heap {
  virtual void* alloc(u32 size, u32 align = 8) noexcept = 0;
  virtual void free(void* ptr) noexcept = 0;
};

} // namespace kuribo::mem

inline void* operator new(u32 size, kuribo::mem::Heap& heap,
                          u32 align = 8) noexcept {
  return heap.alloc(size, align);
}

inline void* operator new[](u32 size, kuribo::mem::Heap& heap,
                            u32 align = 8) noexcept {
  return heap.alloc(size, align);
}

inline void operator delete(void* obj, kuribo::mem::Heap& heap) noexcept {
  heap.free(obj);
}
inline void operator delete[](void* obj, kuribo::mem::Heap& heap) noexcept {
  heap.free(obj);
}

namespace kuribo::mem {

template <typename T> struct default_delete {
  void operator()(T* obj, Heap* heap) {
    if constexpr (!eastl::is_array_v<T>)
      obj->~T();
    heap->free(obj);
  }
};

template <typename TObject, typename TDeleter = default_delete<TObject>>
struct unique_ptr {
  TObject* mObj = nullptr;
  Heap* mHeap = nullptr;
  TDeleter mDeleter{};

  unique_ptr() = default;
  unique_ptr(std::nullptr_t) : mObj(nullptr), mHeap(nullptr) {}
  unique_ptr(TObject* obj, Heap* heap) : mObj(obj), mHeap(heap) {}
  ~unique_ptr() { reset(); }

  unique_ptr(const unique_ptr&) = delete;
  unique_ptr(unique_ptr&& rhs) {
    reset();
    mObj = rhs.mObj;
    mHeap = rhs.mHeap;
    rhs.mObj = nullptr;
  }

  void reset() {
    if (mObj == nullptr)
      return;
    mDeleter(mObj, mHeap);
    mObj = nullptr;
  }

  unique_ptr& operator=(unique_ptr&& rhs) {
    reset();
    mObj = rhs.mObj;
    mHeap = rhs.mHeap;
    rhs.mObj = nullptr;
  }

  TObject* get() { return mObj; }
  const TObject* get() const { return mObj; }

  TObject& operator->() { return *mObj; }
  const TObject& operator->() const { return *mObj; }

  operator bool() const { return mObj != nullptr; }
};

template <typename T, typename... Args>
inline unique_ptr<T, default_delete<T>> make_unique(Heap& heap, u32 align = 8,
                                                    Args... args) {
  return {new (&heap, align) T(args...)), &heap};
}
template <typename T, typename... Args>
inline unique_ptr<T, default_delete<T>> make_unique(Heap& heap, u32 align = 8) {
  return {new (&heap, align) T()), &heap};
}

#if 0
template <typename TObject, bool ThreadSafe = true,
          typename TDeleter = default_delete<TObject>>
class shared_ptr {
  shared_ptr(TObject* obj, Heap& heap) : mObj(obj), mHeap(&heap) {
    mControl = *new (heap) Control();
    // ControlHandle handles ref counting
  }
  shared_ptr(const shared_ptr& rhs)
      : mControl(rhs.mControl), mObj(rhs.mObj), mHeap(rhs.mHeap) {
    // ControlHandle handles ref counting
  }

  ~shared_ptr() {
    mControl->mControl.decrement();
    if (mControl.mControl->isUnused()) {
      mDeleter(mObj, mHeap);
    }
  }

  TObject* get() { return mObj; }
  const TObject* get() const { return mObj; }

  TObject* operator->() { return mObj; }
  const TObject* operator->() const { return mObj; }

public:
  struct Control {
    mutable int mRef = 0;

    void adjust(int offset) const {
      if constexpr (ThreadSafe) {
        Critical g;
        mRef += offset;
      } else {
        mRef += offset;
      }
    }
    void increment() const { adjust(1); }
    void decrement() const { adjust(-1); }

    // If there was a race condition, and we forced a win, it would mean we
    // starve the loser of their resource
    bool isUnused() const { return mRef == 0; }
  };

  struct ControlHandle {
    ControlHandle(Control& control) {
      control.increment();
      mControl = &control;
    }
    ControlHandle(ControlHandle&&) = delete;
    ControlHandle(const ControlHandle& rhs) {
      rhs.mControl->increment();
      mControl = rhs.mControl;
    }
    Control* mControl = nullptr;
  };

private:
  ControlHandle mControl = nullptr;
  TObject* mObj = nullptr;
  Heap* mHeap = nullptr;
  TDeleter mDeleter;
};
#endif

} // namespace kuribo::mem