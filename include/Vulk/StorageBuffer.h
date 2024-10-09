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

class StorageBuffer : public Buffer {
 public:
  enum Property : uint8_t {
    NONE              = 0x00,
    HOST_VISIBLE      = 0x01 << 0,
    AS_VERTEX_BUFFER  = 0x01 << 1
  };

 public:
  StorageBuffer(const Device& device, VkDeviceSize size, Property property = Property::NONE);
  template <typename Element>
  StorageBuffer(const Device& device, const std::vector<Element>& elements) {
    create(device, elements);
  }
  template <typename Element>
  StorageBuffer(const Device& device,
                const CommandBuffer& stagingCommandBuffer,
                const std::vector<Element>& elements) {
    create(device, stagingCommandBuffer, elements);
  }

  // Buffer will be device local by default and can only be loaded using a staging buffer.
  // To make the buffer host visible, use Property::HOST_VISIBLE
  void create(const Device& device, VkDeviceSize size, Property property = Property::NONE);
  // Buffer will be host visible and the data will be copied directly from host to buffer
  template <typename Element>
  void create(const Device& device, const std::vector<Element>& elements);
  // Buffer will be device local only and the data will be copied from host to buffer using a
  // staging buffer
  template <typename Element>
  void create(const Device& device,
              const CommandBuffer& stagingCommandBuffer,
              const std::vector<Element>& elements);

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(StorageBuffer, Buffer);
};

template <typename Element>
inline void StorageBuffer::create(const Device& device, const std::vector<Element>& elements) {
  VkDeviceSize size = sizeof(Element) * elements.size();
  create(device, size, Property::HOST_VISIBLE);
  load(elements.data(), size);
}

template <typename Element>
inline void StorageBuffer::create(const Device& device,
                                 const CommandBuffer& stagingCommandBuffer,
                                 const std::vector<Element>& elements) {
  VkDeviceSize size = sizeof(Element) * elements.size();
  create(device, size);

  const auto& queue = device.queue(Device::QueueFamilyType::Compute);
  load(queue, stagingCommandBuffer, elements.data(), size);
}

MI_ENABLE_ENUM_BITWISE_OP(StorageBuffer::Property);

MI_NAMESPACE_END(Vulk)