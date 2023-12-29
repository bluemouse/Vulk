#include <Vulk/CommandPool.h>

#include <Vulk/Device.h>
#include <Vulk/internal/vulkan_debug.h>

NAMESPACE_BEGIN(Vulk)

CommandPool::CommandPool(const Device& device, uint32_t queueFamilyIndex) {
  create(device, queueFamilyIndex);
}

CommandPool::~CommandPool() {
  if (isCreated()) {
    destroy();
  }
}

void CommandPool::create(const Device& device, uint32_t queueFamilyIndex) {
  MI_VERIFY(!isCreated());

  _device = device.get_weak();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndex;

  MI_VERIFY_VKCMD(vkCreateCommandPool(device, &poolInfo, nullptr, &_pool));

  vkGetDeviceQueue(device, queueFamilyIndex, 0, &_queue);
}

void CommandPool::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyCommandPool(device(), _pool, nullptr);

  _pool   = VK_NULL_HANDLE;
  _queue  = VK_NULL_HANDLE;
  _device.reset();
}

NAMESPACE_END(Vulk)