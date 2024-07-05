#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END(name) }

NAMESPACE_BEGIN(Vulk)

class NotCopyable {
 public:
  NotCopyable()                              = default;
  NotCopyable(const NotCopyable&)            = delete;
  NotCopyable& operator=(const NotCopyable&) = delete;
};

template <typename T>
class Sharable : public std::enable_shared_from_this<T> {
 public:
  using shared_ptr = std::shared_ptr<T>;
  using weak_ptr   = std::weak_ptr<T>;

  using shared_ptr_const = std::shared_ptr<const T>;
  using weak_ptr_const   = std::weak_ptr<const T>;

  using unique_ptr = std::unique_ptr<T>;

  virtual ~Sharable() = default;

  template <class... Args>
  static shared_ptr make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template <class... Args>
  static unique_ptr make_unique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  shared_ptr get_shared() { return std::enable_shared_from_this<T>::shared_from_this(); }
  weak_ptr get_weak() { return std::enable_shared_from_this<T>::weak_from_this(); }

  shared_ptr_const get_shared() const {
    return std::enable_shared_from_this<T>::shared_from_this();
  }
  weak_ptr_const get_weak() const { return std::enable_shared_from_this<T>::weak_from_this(); }
};

// Create a macro to define the following codes
#define MI_DEFINE_SHARED_PTR(type, base)                            \
  using shared_ptr = std::shared_ptr<type>;                         \
  using weak_ptr   = std::weak_ptr<type>;                           \
                                                                    \
  template <class... Args>                                          \
  static shared_ptr make_shared(Args&&... args) {                   \
    return std::make_shared<type>(std::forward<Args>(args)...);     \
  }                                                                 \
                                                                    \
  shared_ptr get_shared() {                                         \
    return std::static_pointer_cast<type>(base::get_shared());      \
  }                                                                 \
  weak_ptr get_weak() {                                             \
    return std::static_pointer_cast<type>(base::get_weak().lock()); \
  }

NAMESPACE_END(Vulk)