#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Buffer.h>

NAMESPACE_Vulk_BEGIN

class Device;
class CommandBuffer;

class VertexBuffer : public Buffer {
 public:
  VertexBuffer() = default;
  VertexBuffer(const Device& device, VkDeviceSize size, bool hostVisible = false);

  // Transfer the ownership from `rhs` to `this`
  VertexBuffer(VertexBuffer&& rhs) = default;
  VertexBuffer& operator=(VertexBuffer&& rhs) noexcept(false) = default;

  // Buffer will be device local and can only be loaded using a staging buffer
  void create(const Device& device, VkDeviceSize size, bool hostVisible = false);
  template <typename Vertex>
  // Buffer will be host visible and the data will be copied directly from host to buffer
  void create(const Device& device, const std::vector<Vertex>& vertices);
  // Buffer will be device local only and the data will be copied from host to buffer using a
  // staging buffer
  template <typename Vertex>
  void create(const Device& device,
              const CommandBuffer& stagingCommandBuffer,
              const std::vector<Vertex>& vertices);
};

template <typename Vertex>
inline void VertexBuffer::create(const Device& device, const std::vector<Vertex>& vertices) {
  VkDeviceSize size = sizeof(Vertex) * vertices.size();
  create(device, size, true);
  load(vertices.data(), size);
}

template <typename Vertex>
inline void VertexBuffer::create(const Device& device,
                                 const CommandBuffer& stagingCommandBuffer,
                                 const std::vector<Vertex>& vertices) {
  VkDeviceSize size = sizeof(Vertex) * vertices.size();
  create(device, size);
  load(stagingCommandBuffer, vertices.data(), size);
}

NAMESPACE_Vulk_END