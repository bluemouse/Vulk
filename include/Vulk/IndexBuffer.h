#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Buffer.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;

class IndexBuffer : public Buffer {
 public:
  IndexBuffer() = default;
  IndexBuffer(const Device& device, VkDeviceSize size, bool hostVisible = false);

  // Transfer the ownership from `rhs` to `this`
  IndexBuffer(IndexBuffer&& rhs)                            = default;
  IndexBuffer& operator=(IndexBuffer&& rhs) noexcept(false) = default;

  // Buffer will be device local and can only be loaded using a staging buffer
  void create(const Device& device, VkDeviceSize size, bool hostVisible = false);
  template <typename Index>
  // Buffer will be host visible and the data will be copied directly from host to buffer
  void create(const Device& device, const std::vector<Index>& indices);
  // Buffer will be device local only and the data will be copied from host to buffer using a
  // staging buffer
  template <typename Index>
  void create(const Device& device,
              const CommandBuffer& stagingCommandBuffer,
              const std::vector<Index>& indices);
};

template <typename Index>
inline void IndexBuffer::create(const Device& device, const std::vector<Index>& indices) {
  VkDeviceSize size = sizeof(Index) * indices.size();
  create(device, size, true);
  load(indices.data(), size);
}

template <typename Index>
inline void IndexBuffer::create(const Device& device,
                                const CommandBuffer& stagingCommandBuffer,
                                const std::vector<Index>& indices) {
  VkDeviceSize size = sizeof(Index) * indices.size();
  create(device, size);
  load(stagingCommandBuffer, indices.data(), size);
}

NAMESPACE_END(Vulk)