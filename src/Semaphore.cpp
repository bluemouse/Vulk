#include <Vulk/Semaphore.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

Semaphore::Semaphore(const Device& device) {
  create(device);
}

Semaphore::~Semaphore() {
  if (isCreated()) {
    destroy();
  }
}

void Semaphore::create(const Device& device) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  MI_VERIFY_VK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_semaphore));
}

void Semaphore::destroy() {
  MI_VERIFY(isCreated());
  vkDestroySemaphore(device(), _semaphore, nullptr);

  _semaphore = VK_NULL_HANDLE;
  _device.reset();
}

MI_NAMESPACE_END(Vulk)
