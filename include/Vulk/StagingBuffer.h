#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#include <Vulk/Buffer.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;
class Queue;
class Image;

class StagingBuffer : public Buffer {
 public:
  StagingBuffer(const Device& device, VkDeviceSize size, const void* data = nullptr);

  void create(const Device& device, VkDeviceSize size);

  void copyFromHost(const void* src, VkDeviceSize size) { copyFromHost(src, 0, size); }
  void copyFromHost(const void* src, VkDeviceSize offset, VkDeviceSize size);

  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    const VkBufferCopy& roi,
                    const std::vector<Semaphore*>& waits   = {},
                    const std::vector<Semaphore*>& signals = {},
                    const Fence& fence                     = {}) const;
  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    VkDeviceSize size,
                    const std::vector<Semaphore*>& waits   = {},
                    const std::vector<Semaphore*>& signals = {},
                    const Fence& fence                     = {}) const;
  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   const VkBufferImageCopy& roi,
                   const std::vector<Semaphore*>& waits   = {},
                   const std::vector<Semaphore*>& signals = {},
                   const Fence& fence                     = {}) const;
  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   uint32_t width,
                   uint32_t height,
                   const std::vector<Semaphore*>& waits   = {},
                   const std::vector<Semaphore*>& signals = {},
                   const Fence& fence                     = {}) const;

  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    const VkBufferCopy& roi,
                    const Fence& fence) const {
    copyToBuffer(commandBuffer, dst, roi, {}, {}, fence);
  }
  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    VkDeviceSize size,
                    const Fence& fence) const {
    copyToBuffer(commandBuffer, dst, size, {}, {}, fence);
  }
  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   const VkBufferImageCopy& roi,
                   const Fence& fence) const {
    copyToImage(commandBuffer, dst, roi, {}, {}, fence);
  }
  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   uint32_t width,
                   uint32_t height,
                   const Fence& fence) const {
    copyToImage(commandBuffer, dst, width, height, {}, {}, fence);
  }

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(StagingBuffer, Buffer);
};

MI_NAMESPACE_END(Vulk)