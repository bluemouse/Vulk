#include <Vulk/Queue.h>

#include <Vulk/Device.h>
#include <Vulk/internal/debug.h>

NAMESPACE_BEGIN(Vulk)

Queue::Queue(const Device& device, uint32_t queueFamilyIndex)
: _queueFamilyIndex(queueFamilyIndex) {
  // According to doc: vkGetDeviceQueue must only be used to get queues that
  // were created with the flags parameter of VkDeviceQueueCreateInfo set to
  // zero. To get queues that were created with a non-zero flags parameter use
  // vkGetDeviceQueue2.
  //
  // Currently we are not using flags parameter so we can use vkGetDeviceQueue.

  // `queueIndex` must be less than the value of `VkDeviceQueueCreateInfo::queueCount`
  // for the queue family indicated by queueFamilyIndex when device was created.
  // It was `1` hence we have to use `0` as queueIndex.
  _queueIndex = 0;
  vkGetDeviceQueue(device, queueFamilyIndex, _queueIndex, &_queue);
}

Queue::~Queue() {}

NAMESPACE_END(Vulk)