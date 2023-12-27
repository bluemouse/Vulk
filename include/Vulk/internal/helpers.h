#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

inline bool operator==(const VkExtent2D& lhs, const VkExtent2D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator==(const VkExtent3D& lhs, const VkExtent3D& rhs) {
  return lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth;
}

NAMESPACE_END(Vulk)