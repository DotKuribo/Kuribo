#pragma once

#include <core/sync.hxx>
#include <utility>

namespace kuribo {

inline u32 GetCanary(void* pObj) { return reinterpret_cast<u32>(pObj) ^ 191; }

template <typename T> struct CanaryObject {
  CanaryObject(const CanaryObject& rhs) = delete;
  CanaryObject(CanaryObject&& rhs) {
    rhs.check();
    mObj = std::move(rhs);
    mCanary = GetCanary(this);
  }

  T mObj;
  u32 mCanary;

  T& get() { return mObj; }
  const T& get() const { return mObj; }

  T& operator->() { return mObj; }
  const T& operator->() const { return mObj; }

  [[nodiscard]] bool valid() const { return mCanary == GetCanary(this); }
  void check() const {
    if (!valid()) {
      Critical g;
      for (;;) {
        // trap
      }
    }
  }

  CanaryObject() : mCanary(GetCanary(this)) {}
  ~CanaryObject() { check(); }
};

} // namespace kuribo