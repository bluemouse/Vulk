#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Buffer.h>

NAMESPACE_VULKAN_BEGIN

class Device;

class VertexBuffer : public Buffer {
 public:
  VertexBuffer() = default;
  VertexBuffer(const Device& device, VkDeviceSize size);

  // Transfer the ownership from `rhs` to `this`
  VertexBuffer(VertexBuffer&& rhs) = default;
  VertexBuffer& operator=(VertexBuffer&& rhs) noexcept(false) = default;

  void create(const Device& device, VkDeviceSize size);
};

NAMESPACE_VULKAN_END