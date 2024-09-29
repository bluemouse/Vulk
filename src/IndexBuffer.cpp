#include <Vulk/IndexBuffer.h>

MI_NAMESPACE_BEGIN(Vulk)

IndexBuffer::IndexBuffer(const Device& device, VkDeviceSize size, bool hostVisible) {
  create(device, size, hostVisible);
}

void IndexBuffer::create(const Device& device, VkDeviceSize size, bool hostVisible) {
  VkBufferUsageFlags usage         = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  if (hostVisible) {
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  Buffer::create(device, size, usage);
  Buffer::allocate(properties);
}

MI_NAMESPACE_END(Vulk)
