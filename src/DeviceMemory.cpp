#include <Vulk/DeviceMemory.h>

#include <Vulk/internal/vulkan_debug.h>

#include <Vulk/Device.h>
#include <Vulk/PhysicalDevice.h>

NAMESPACE_BEGIN(Vulk)

DeviceMemory::DeviceMemory(const Device& device,
                           VkMemoryPropertyFlags properties,
                           const VkMemoryRequirements& requirements) {
  allocate(device, properties, requirements);
}

DeviceMemory::~DeviceMemory() {
  if (isAllocated()) {
    free();
  }
}

void DeviceMemory::allocate(const Device& device,
                            VkMemoryPropertyFlags properties,
                            const VkMemoryRequirements& requirements) {
  MI_VERIFY(!isAllocated());
  _device = device.get_weak();
  _size   = requirements.size;

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = requirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);

  MI_VERIFY_VKCMD(vkAllocateMemory(device, &allocInfo, nullptr, &_memory));

  _hostVisible = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
}

void DeviceMemory::free() {
  MI_VERIFY(isAllocated());

  vkFreeMemory(device(), _memory, nullptr);

  _memory       = VK_NULL_HANDLE;
  _size         = 0;
  _hostVisible  = false;
  _mappedMemory = nullptr;
  _device.reset();
}

void* DeviceMemory::map(VkDeviceSize offset, VkDeviceSize size) {
  MI_VERIFY(isAllocated());
  MI_VERIFY(isHostVisible());
  MI_VERIFY(!isMapped());

  vkMapMemory(device(), _memory, offset, size, 0, &_mappedMemory);
  return _mappedMemory;
}

void DeviceMemory::unmap() {
  MI_VERIFY(isMapped());
  vkUnmapMemory(device(), _memory);
  _mappedMemory = nullptr;
}

uint32_t DeviceMemory::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(device().physicalDevice(), &memProperties);

  uint32_t idx = 0;
  for (auto& memType : memProperties.memoryTypes) {
    if ((typeFilter & (1 << idx)) != 0 && (memType.propertyFlags & properties) == properties) {
      return idx;
    }
    ++idx;
  }
  throw std::runtime_error("Failed to find suitable memory type!");
}

NAMESPACE_END(Vulk)