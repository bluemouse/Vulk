#include <Vulk/Fence.h>

#include <Vulk/Device.h>

NAMESPACE_Vulk_BEGIN

Fence::Fence(const Device& device, bool signaled) {
  create(device, signaled);
}

Fence::~Fence() {
  if (isCreated()) {
    destroy();
  }
}

Fence::Fence(Fence&& rhs) noexcept {
  moveFrom(rhs);
}

Fence& Fence::operator=(Fence&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void Fence::moveFrom(Fence& rhs) {
  MI_VERIFY(!isCreated());
  _fence = rhs._fence;
  _device = rhs._device;

  rhs._fence = VK_NULL_HANDLE;
  rhs._device = nullptr;
}

void Fence::create(const Device& device, bool signaled) {
  MI_VERIFY(!isCreated());
  _device = &device;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (signaled) {
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  MI_VERIFY_VKCMD(vkCreateFence(device, &fenceInfo, nullptr, &_fence));
}

void Fence::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFence(*_device, _fence, nullptr);

  _device = nullptr;
  _fence = VK_NULL_HANDLE;
}

void Fence::wait(uint64_t timeout) const {
  MI_VERIFY(isCreated());
  MI_VERIFY_VKCMD(vkWaitForFences(*_device, 1, &_fence, VK_TRUE, timeout));
}

void Fence::reset() {
  MI_VERIFY(isCreated());
  MI_VERIFY_VKCMD(vkResetFences(*_device, 1, &_fence));
}

NAMESPACE_Vulk_END
