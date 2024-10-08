#pragma once

#include <Vulk/internal/base.h>

#define MI_INIT_VKPROC(cmd)                                                       \
  auto cmd = reinterpret_cast<PFN_##cmd>(vkGetInstanceProcAddr(_instance, #cmd)); \
  MI_VERIFY(cmd != nullptr);

#define MI_ENABLE_ENUM_BITWISE_OP(enum_t)                                         \
  constexpr enum_t operator|(const enum_t& lhs, const enum_t& rhs) {              \
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) | \
                               static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                                               \
  constexpr enum_t operator&(const enum_t& lhs, const enum_t& rhs) {              \
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) & \
                               static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                                               \
  constexpr enum_t operator^(const enum_t& lhs, const enum_t& rhs) {              \
    return static_cast<enum_t>(static_cast<std::underlying_type_t<enum_t>>(lhs) ^ \
                               static_cast<std::underlying_type_t<enum_t>>(rhs)); \
  }                                                                               \
  constexpr bool operator!(const enum_t& val) {                                   \
    return static_cast<std::underlying_type_t<enum_t>>(val) ==                    \
           static_cast<std::underlying_type_t<enum_t>>(0);                        \
  }

MI_NAMESPACE_BEGIN(Vulk)

inline bool operator==(const VkExtent2D& lhs, const VkExtent2D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator==(const VkExtent3D& lhs, const VkExtent3D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth;
}

template <class Container, typename Element>
bool contain(const Container& container, const Element& element) {
  return std::find(container.begin(), container.end(), element) != container.end();
}

MI_NAMESPACE_END(Vulk)