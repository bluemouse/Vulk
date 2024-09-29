#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>
#include <Vulk/internal/helpers.h>

#include <Vulk/Device.h>
#include <Vulk/Queue.h>
#include <Vulk/DeviceMemory.h>
#include <Vulk/Buffer.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;

class VertexBuffer : public Buffer {
 public:
  enum Property : uint8_t {
    NONE              = 0x00,
    HOST_VISIBLE      = 0x01 << 0,
    AS_STORAGE_BUFFER = 0x01 << 1
  };

 public:
  VertexBuffer(const Device& device, VkDeviceSize size, Property property = Property::NONE);
  template <typename Vertex>
  VertexBuffer(const Device& device, const std::vector<Vertex>& vertices) {
    create(device, vertices);
  }
  template <typename Vertex>
  VertexBuffer(const Device& device,
               const CommandBuffer& stagingCommandBuffer,
               const std::vector<Vertex>& vertices) {
    create(device, stagingCommandBuffer, vertices);
  }

  // Buffer will be device local by default and can only be loaded using a staging buffer.
  // To make the buffer host visible, use Property::HOST_VISIBLE
  void create(const Device& device, VkDeviceSize size, Property property = Property::NONE);
  // Buffer will be host visible and the data will be copied directly from host to buffer
  template <typename Vertex>
  void create(const Device& device, const std::vector<Vertex>& vertices);
  // Buffer will be device local only and the data will be copied from host to buffer using a
  // staging buffer
  template <typename Vertex>
  void create(const Device& device,
              const CommandBuffer& stagingCommandBuffer,
              const std::vector<Vertex>& vertices);

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(VertexBuffer, Buffer);
};

template <typename Vertex>
inline void VertexBuffer::create(const Device& device, const std::vector<Vertex>& vertices) {
  VkDeviceSize size = sizeof(Vertex) * vertices.size();
  create(device, size, Property::HOST_VISIBLE);
  load(vertices.data(), size);
}

template <typename Vertex>
inline void VertexBuffer::create(const Device& device,
                                 const CommandBuffer& stagingCommandBuffer,
                                 const std::vector<Vertex>& vertices) {
  VkDeviceSize size = sizeof(Vertex) * vertices.size();
  create(device, size);

  const auto& queue = device.queue(Vulk::Device::QueueFamilyType::Graphics);
  load(queue, stagingCommandBuffer, vertices.data(), size);
}

MI_ENABLE_ENUM_BITWISE_OP(VertexBuffer::Property);

MI_NAMESPACE_END(Vulk)