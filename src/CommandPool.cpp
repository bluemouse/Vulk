#include <Vulk/CommandPool.h>

#include <Vulk/Device.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

CommandPool::CommandPool(const Device& device, uint32_t queueFamilyIndex) {
  create(device, queueFamilyIndex);
}

CommandPool::~CommandPool() {
  if (isCreated()) {
    destroy();
  }
}

CommandPool::CommandPool(CommandPool&& rhs) noexcept {
  moveFrom(rhs);
}

CommandPool& CommandPool::operator=(CommandPool&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void CommandPool::moveFrom(CommandPool& rhs) {
  MI_VERIFY(!isCreated());
  _pool   = rhs._pool;
  _queue  = rhs._queue;
  _device = rhs._device;

  rhs._pool   = VK_NULL_HANDLE;
  rhs._queue  = VK_NULL_HANDLE;
  rhs._device = nullptr;
}

void CommandPool::create(const Device& device, uint32_t queueFamilyIndex) {
  MI_VERIFY(!isCreated());

  _device = &device;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndex;

  MI_VERIFY_VKCMD(vkCreateCommandPool(device, &poolInfo, nullptr, &_pool));

  vkGetDeviceQueue(device, queueFamilyIndex, 0, &_queue);
}

void CommandPool::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyCommandPool(*_device, _pool, nullptr);
  _pool   = VK_NULL_HANDLE;
  _queue  = VK_NULL_HANDLE;
  _device = nullptr;
}

NAMESPACE_END(Vulk)