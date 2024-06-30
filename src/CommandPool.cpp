#include <Vulk/CommandPool.h>

#include <Vulk/Device.h>
#include <Vulk/internal/debug.h>

NAMESPACE_BEGIN(Vulk)

CommandPool::CommandPool(const Device& device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
  create(device, queueFamilyIndex, flags);
}

CommandPool::~CommandPool() {
  if (isCreated()) {
    destroy();
  }
}

void CommandPool::create(const Device& device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
  MI_VERIFY(!isCreated());

  _device = device.get_weak();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = flags;
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