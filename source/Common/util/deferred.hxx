#pragma once

#include "config.h"
#include "types.h"

namespace kuribo {

template <typename T> struct DeferredInitialization {
  // TODO: Avoid initialization function pointer chain at all cost..
  u8 data[sizeof(T)]{0};

  u8 initd{0}; // This might cause issues
  //	constexpr DeferredInitialization() = default;
  //	constexpr ~DeferredInitialization() = default;
  bool isInitialized() const { return initd; }
  template <typename... Args> void initialize(Args... args) {
    new ((void*)&data) T(args...);
    initd = 1;
  }
  void deinitialize() {
    ((T&)*this).~T();
    initd = 0;
  }

  operator T&() { return *reinterpret_cast<T*>(&data); }
  operator const T&() const { return *reinterpret_cast<const T*>(&data); }

  T* operator->() { return reinterpret_cast<T*>(&data); }
  const T* operator->() const { return reinterpret_cast<T*>(&data); }
};
template <typename T> struct DeferredSingleton {
  DeferredInitialization<T> sInstance;

  T& getInstance() { return sInstance; }
  const T& getInstanceConst() { return sInstance; }
  template <typename... Args> bool initializeStaticInstance(Args... args) {
    if (sInstance.isInitialized())
      return false;
    sInstance.initialize(args...);
    return true;
  }
  template <typename... Args> bool deinitializeStaticInstance(Args... args) {
    if (!sInstance.isInitialized())
      return false;
    sInstance.deinitialize(args...);
    return true;
  }
  bool isStaticInstanceInitialized() { return sInstance.isInitialized(); }
};

} // namespace kuribo