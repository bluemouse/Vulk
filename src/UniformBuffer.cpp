#include <Vulk/UniformBuffer.h>

#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

UniformBuffer::UniformBuffer(const Device& device, VkDeviceSize size) {
  create(device, size);
}

void UniformBuffer::create(const Device& device, VkDeviceSize size) {
  Buffer::create(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  Buffer::allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

MI_NAMESPACE_END(Vulk)
