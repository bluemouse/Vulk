#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Buffer.h>

NAMESPACE_VULKAN_BEGIN

class Device;

class IndexBuffer : public Buffer {
 public:
  IndexBuffer() = default;
  IndexBuffer(const Device& device, VkDeviceSize size);

  // Transfer the ownership from `rhs` to `this`
  IndexBuffer(IndexBuffer&& rhs) = default;
  IndexBuffer& operator=(IndexBuffer&& rhs) noexcept(false) = default;

  void create(const Device& device, VkDeviceSize size);
};

NAMESPACE_VULKAN_END