#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>
#include <Vulk/internal/vulkan_debug.h>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS
#include <glm/glm.hpp>

NAMESPACE_BEGIN(Vulk)

template <typename T>
struct ImageTrait {
  static constexpr VkFormat format    = VK_FORMAT_UNDEFINED;
  static constexpr uint32_t size      = 0;
  static constexpr uint32_t dimension = 0;
}; // NAMESPACE_BEGIN(Vulk)
template <>
struct ImageTrait<float> {
  [[maybe_unused]] static constexpr VkFormat format    = VK_FORMAT_R32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size      = sizeof(float);
  [[maybe_unused]] static constexpr uint32_t dimension = 1;
};
template <>
struct ImageTrait<glm::vec2> {
  [[maybe_unused]] static constexpr VkFormat format    = VK_FORMAT_R32G32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size      = sizeof(glm::vec2);
  [[maybe_unused]] static constexpr uint32_t dimension = 2;
};
template <>
struct ImageTrait<glm::vec3> {
  [[maybe_unused]] static constexpr VkFormat format    = VK_FORMAT_R32G32B32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size      = sizeof(glm::vec3);
  [[maybe_unused]] static constexpr uint32_t dimension = 3;
};
template <>
struct ImageTrait<glm::vec4> {
  [[maybe_unused]] static constexpr VkFormat format    = VK_FORMAT_R32G32B32A32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size      = sizeof(glm::vec4);
  [[maybe_unused]] static constexpr uint32_t dimension = 4;
};

template <typename I>
struct IndexTrait {
  static constexpr VkIndexType type = VK_INDEX_TYPE_NONE_KHR;
};
template <>
struct IndexTrait<uint8_t> {
  static constexpr VkIndexType type = VK_INDEX_TYPE_UINT8_EXT;
};
template <>
struct IndexTrait<uint16_t> {
  static constexpr VkIndexType type = VK_INDEX_TYPE_UINT16;
};
template <>
struct IndexTrait<uint32_t> {
  static constexpr VkIndexType type = VK_INDEX_TYPE_UINT32;
};

struct FormatInfo {
  // Return the number of bytes of the given format
  static uint32_t size(VkFormat format);
};

NAMESPACE_END(Vulk)

#define formatof(var) Vulk::ImageTrait<decltype(var)>::format
#define formatsizeof(format) Vulk::FormatInfo::size(format)
