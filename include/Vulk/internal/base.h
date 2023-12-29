#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/debug.h>

#include <memory>

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END(name) }

#define MI_INIT_VKPROC(cmd)                                                       \
  auto cmd = reinterpret_cast<PFN_##cmd>(vkGetInstanceProcAddr(_instance, #cmd)); \
  MI_VERIFY(cmd != nullptr);

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

 using shared_const_ptr = std::shared_ptr<const T>;
 using weak_const_ptr   = std::weak_ptr<const T>;

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

 shared_const_ptr get_shared() const { return std::enable_shared_from_this<T>::shared_from_this(); }
 weak_const_ptr get_weak() const { return std::enable_shared_from_this<T>::weak_from_this(); }
};

NAMESPACE_END(Vulk)