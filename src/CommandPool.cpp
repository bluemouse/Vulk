#include <Vulk/CommandPool.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Queue.h>

#include <Vulk/internal/debug.h>

NAMESPACE_BEGIN(Vulk)

CommandPool::CommandPool(const Device& device,
                         Device::QueueFamilyType queueFamilyType,
                         Mode modes) {
  create(device, queueFamilyType, modes);
}

CommandPool::~CommandPool() {
  if (isCreated()) {
    destroy();
  }
}

void CommandPool::create(const Device& device,
                         Device::QueueFamilyType queueFamilyType,
                         Mode modes) {
  MI_VERIFY(!isCreated());

  const auto& queue = device.queue(queueFamilyType);

  _device = device.get_weak();
  _queue  = queue.get_weak();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = static_cast<VkCommandPoolCreateFlags>(modes);
  poolInfo.queueFamilyIndex = queue.queueFamilyIndex();

  MI_VERIFY_VKCMD(vkCreateCommandPool(device, &poolInfo, nullptr, &_pool));
}

void CommandPool::destroy() {
  destroy(device());
}

void CommandPool::destroy(VkDevice device) {
  MI_VERIFY(isCreated());

  vkDestroyCommandPool(device, _pool, nullptr);

  _pool = VK_NULL_HANDLE;
  _device.reset();
}

std::shared_ptr<CommandBuffer> CommandPool::allocatePrimary() const {
  MI_VERIFY(isCreated());
  return CommandBuffer::make_shared(*this, CommandBuffer::Level::Primary);
}

std::shared_ptr<CommandBuffer> CommandPool::allocateSecondary() const {
  MI_VERIFY(isCreated());
  return CommandBuffer::make_shared(*this, CommandBuffer::Level::Secondary);
}

NAMESPACE_END(Vulk)