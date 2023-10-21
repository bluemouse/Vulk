#pragma once

#include <vulkan/vulkan.h>

NAMESPACE_VULKAN_BEGIN

template <typename T>
struct ImageTrait {
  static constexpr VkFormat format = VK_FORMAT_UNDEFINED;
  static constexpr uint32_t size = 0;
  static constexpr uint32_t dimension = 0;
};
template <>
struct ImageTrait<float> {
  [[maybe_unused]] static constexpr VkFormat format = VK_FORMAT_R32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size = sizeof(float);
  [[maybe_unused]] static constexpr uint32_t dimension = 1;
};
template <>
struct ImageTrait<glm::vec2> {
  [[maybe_unused]] static constexpr VkFormat format = VK_FORMAT_R32G32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size = sizeof(glm::vec2);
  [[maybe_unused]] static constexpr uint32_t dimension = 2;
};
template <>
struct ImageTrait<glm::vec3> {
  [[maybe_unused]] static constexpr VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size = sizeof(glm::vec3);
  [[maybe_unused]] static constexpr uint32_t dimension = 3;
};
template <>
struct ImageTrait<glm::vec4> {
  [[maybe_unused]] static constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
  [[maybe_unused]] static constexpr uint32_t size = sizeof(glm::vec4);
  [[maybe_unused]] static constexpr uint32_t dimension = 4;
};

NAMESPACE_VULKAN_END

#define formatof(var) Vulkan::ImageTrait<decltype(var)>::format
