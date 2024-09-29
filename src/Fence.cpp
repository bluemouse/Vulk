#include <Vulk/Fence.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

Fence::Fence(const Device& device, bool signaled) {
  create(device, signaled);
}

Fence::~Fence() {
  if (isCreated()) {
    destroy();
  }
}

void Fence::create(const Device& device, bool signaled) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (signaled) {
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  MI_VERIFY_VK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &_fence));
}

void Fence::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFence(device(), _fence, nullptr);

  _fence = VK_NULL_HANDLE;
  _device.reset();
}

void Fence::wait(uint64_t timeout) const {
  MI_VERIFY(isCreated());
  MI_VERIFY_VK_RESULT(vkWaitForFences(device(), 1, &_fence, VK_TRUE, timeout));
}

void Fence::reset() {
  MI_VERIFY(isCreated());
  MI_VERIFY_VK_RESULT(vkResetFences(device(), 1, &_fence));
}

MI_NAMESPACE_END(Vulk)
