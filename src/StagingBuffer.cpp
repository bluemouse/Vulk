#include <Vulk/StagingBuffer.h>

#include <cstring>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Queue.h>
#include <Vulk/Image.h>

#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

StagingBuffer::StagingBuffer(const Device& device, VkDeviceSize size, const void* data) {
  create(device, size);
  if (data) {
    copyFromHost(data, size);
  }
}

void StagingBuffer::create(const Device& device, VkDeviceSize size) {
  Buffer::create(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  Buffer::allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void StagingBuffer::copyFromHost(const void* src, VkDeviceSize offset, VkDeviceSize size) {
  MI_VERIFY(size <= this->size());
  void* data = map(offset, size);
  std::memcpy(data, src, size);
  unmap();
}

void StagingBuffer::copyToBuffer(const CommandBuffer& commandBuffer,
                                 Buffer& dst,
                                 const VkBufferCopy& roi,
                                 const std::vector<Semaphore*>& waits,
                                 const std::vector<Semaphore*>& signals,
                                 const Fence& fence) const {
  commandBuffer.beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    vkCmdCopyBuffer(commandBuffer, *this, dst, 1, &roi);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(waits, signals, fence);
}

void StagingBuffer::copyToBuffer(const CommandBuffer& commandBuffer,
                                 Buffer& dst,
                                 VkDeviceSize size,
                                 const std::vector<Semaphore*>& waits,
                                 const std::vector<Semaphore*>& signals,
                                 const Fence& fence) const {
  copyToBuffer(commandBuffer, dst, {0, 0, size}, waits, signals, fence);
}

void StagingBuffer::copyToImage(const CommandBuffer& commandBuffer,
                                Image& dst,
                                const VkBufferImageCopy& roi,
                                const std::vector<Semaphore*>& waits,
                                const std::vector<Semaphore*>& signals,
                                const Fence& fence) const {
  commandBuffer.beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    vkCmdCopyBufferToImage(commandBuffer, *this, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &roi);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(waits, signals, fence);
}

void StagingBuffer::copyToImage(const CommandBuffer& commandBuffer,
                                Image& dst,
                                uint32_t width,
                                uint32_t height,
                                const std::vector<Semaphore*>& waits,
                                const std::vector<Semaphore*>& signals,
                                const Fence& fence) const {
  copyToImage(commandBuffer,
              dst,
              {0, 0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0}, {width, height, 1}},
              waits,
              signals,
              fence);
}

MI_NAMESPACE_END(Vulk)
