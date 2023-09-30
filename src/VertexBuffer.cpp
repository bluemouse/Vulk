#include <Vulk/VertexBuffer.h>

NAMESPACE_VULKAN_BEGIN

VertexBuffer::VertexBuffer(const Device& device, VkDeviceSize size) {
  create(device, size);
}

void VertexBuffer::create(const Device& device, VkDeviceSize size) {
  auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  Buffer::create(device, size, usage);
  Buffer::allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

NAMESPACE_VULKAN_END
