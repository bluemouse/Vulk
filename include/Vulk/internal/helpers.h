#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#define MI_INIT_VKPROC(cmd)                                                       \
  auto cmd = reinterpret_cast<PFN_##cmd>(vkGetInstanceProcAddr(_instance, #cmd)); \
  MI_VERIFY(cmd != nullptr);

NAMESPACE_BEGIN(Vulk)

inline bool operator==(const VkExtent2D& lhs, const VkExtent2D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator==(const VkExtent3D& lhs, const VkExtent3D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth;
}

NAMESPACE_END(Vulk)