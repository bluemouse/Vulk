#include <Vulk/Queue.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>
#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

Queue::Queue(const Device& device, Device::QueueFamilyType queueFamily, uint32_t queueFamilyIndex)
    : _queueFamily(queueFamily), _queueFamilyIndex(queueFamilyIndex) {
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

Queue::~Queue() {
}

void Queue::submitCommands(const CommandBuffer& commandBuffer,
                           const std::vector<Semaphore*>& waits,
                           const std::vector<Semaphore*>& signals,
                           const Fence& fence) const {
  VkSubmitInfo submitInfo{};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = commandBuffer;

  std::vector<VkSemaphore> waitSemaphores;
  std::vector<VkPipelineStageFlags> waitStages;
  if (!waits.empty()) {
    waitSemaphores.reserve(waits.size());
    for (const auto& wait : waits) {
      waitSemaphores.push_back(*wait);
      waitStages.push_back(
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT); // TODO: how about other stages such as
                                               // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    }

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores    = waitSemaphores.data();
    submitInfo.pWaitDstStageMask  = waitStages.data();
  }
  std::vector<VkSemaphore> signalSemaphores;
  if (!signals.empty()) {
    signalSemaphores.reserve(signals.size());
    for (const auto& signal : signals) {
      signalSemaphores.push_back(*signal);
    }

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores    = signalSemaphores.data();
  }
  MI_VERIFY_VK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, fence));
}

void Queue::waitIdle() const {
  vkQueueWaitIdle(_queue);
}

void Queue::beginLabel(const char* label, const glm::vec4& color) const {
  if (vkQueueBeginDebugUtilsLabelEXT) {
    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName = label;
    labelInfo.color[0]   = color.r;
    labelInfo.color[1]   = color.g;
    labelInfo.color[2]   = color.b;
    labelInfo.color[3]   = color.a;
    vkQueueBeginDebugUtilsLabelEXT(_queue, &labelInfo);
  }
}
void Queue::insertLabel(const char* label, const glm::vec4& color) const {
  if (vkQueueInsertDebugUtilsLabelEXT) {
    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName = label;
    labelInfo.color[0]   = color.r;
    labelInfo.color[1]   = color.g;
    labelInfo.color[2]   = color.b;
    labelInfo.color[3]   = color.a;
    vkQueueInsertDebugUtilsLabelEXT(_queue, &labelInfo);
  }
}
void Queue::endLabel() const {
  if (vkQueueEndDebugUtilsLabelEXT) {
    vkQueueEndDebugUtilsLabelEXT(_queue);
  }
}
auto Queue::scopedLabel(const char* label, const glm::vec4& color) const
    -> std::unique_ptr<ScopedLabel> {
  return std::make_unique<ScopedLabel>(*this, label, color);
}

MI_NAMESPACE_END(Vulk)
