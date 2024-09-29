#include <Vulk/StorageBuffer.h>

MI_NAMESPACE_BEGIN(Vulk)

StorageBuffer::StorageBuffer(const Device& device, VkDeviceSize size, Property property) {
  create(device, size, property);
}

void StorageBuffer::create(const Device& device, VkDeviceSize size, Property property) {
  VkBufferUsageFlags usage         = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  VkMemoryPropertyFlags properties = 0;

  if (property & Property::HOST_VISIBLE) {
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  if (property & Property::AS_VERTEX_BUFFER) {
    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }

  Buffer::create(device, size, usage);
  Buffer::allocate(properties);
}

MI_NAMESPACE_END(Vulk)
