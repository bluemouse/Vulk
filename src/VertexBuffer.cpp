#include <Vulk/VertexBuffer.h>

NAMESPACE_Vulk_BEGIN

VertexBuffer::VertexBuffer(const Device& device, VkDeviceSize size, bool hostVisible) {
  create(device, size, hostVisible);
}

void VertexBuffer::create(const Device& device, VkDeviceSize size, bool hostVisible) {
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  if (hostVisible) {
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  Buffer::create(device, size, usage);
  Buffer::allocate(properties);
}

NAMESPACE_Vulk_END
