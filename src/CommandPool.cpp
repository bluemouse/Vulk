#include <Vulk/CommandPool.h>

#include <Vulk/Device.h>
#include <Vulk/internal/debug.h>

NAMESPACE_BEGIN(Vulk)

CommandPool::CommandPool(const Device& device,
                         Device::QueueFamilyType queueFamilyType,
                         VkCommandPoolCreateFlags flags) {
  create(device, queueFamilyType, flags);
}

CommandPool::~CommandPool() {
  if (isCreated()) {
    destroy();
  }
}

void CommandPool::create(const Device& device,
                         Device::QueueFamilyType queueFamilyType,
                         VkCommandPoolCreateFlags flags) {
  MI_VERIFY(!isCreated());

  _device = device.get_weak();
  _queueFamilyType = queueFamilyType;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = flags;
  poolInfo.queueFamilyIndex = device.queueFamilyIndex(queueFamilyType).value();

  MI_VERIFY_VKCMD(vkCreateCommandPool(device, &poolInfo, nullptr, &_pool));
}

void CommandPool::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyCommandPool(device(), _pool, nullptr);

  _pool   = VK_NULL_HANDLE;
  _device.reset();
}

NAMESPACE_END(Vulk)