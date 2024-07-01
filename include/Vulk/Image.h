#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <Vulk/DeviceMemory.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;
class Queue;
class StagingBuffer;
class DeviceMemory;

class Image : public Sharable<Image>, private NotCopyable {
 public:
  Image() = default;
  virtual ~Image() override;

  virtual void destroy();

  virtual void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  virtual void free();

  virtual void bind(DeviceMemory& memory, VkDeviceSize offset = 0);

  virtual void* map();
  virtual void* map(VkDeviceSize offset, VkDeviceSize size);
  virtual void unmap();

  virtual void copyFrom(const Queue& queue, const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer);
  virtual void copyFrom(const Queue& queue, const CommandBuffer& cmdBuffer, const Image& srcImage);
  virtual void blitFrom(const Queue& queue, const CommandBuffer& cmdBuffer, const Image& srcImage);

  void transitToNewLayout(const Queue& queue,
                          const CommandBuffer& commandBuffer,
                          VkImageLayout newLayout,
                          bool waitForFinish = true) const;

  void makeShaderReadable(const Queue& queue, const CommandBuffer& cmdBuffer) const;

  operator VkImage() const { return _image; }

  [[nodiscard]] VkImageType type() const { return _type; }
  [[nodiscard]] VkFormat format() const { return _format; }
  [[nodiscard]] VkExtent3D extent() const { return _extent; }

  [[nodiscard]] uint32_t width() const { return _extent.width; }
  [[nodiscard]] uint32_t height() const { return _extent.height; }
  [[nodiscard]] uint32_t depth() const { return _extent.depth; }

  [[nodiscard]] bool isCreated() const { return _image != VK_NULL_HANDLE; }
  [[nodiscard]] bool isAllocated() const {
    return isCreated() && (_memory && _memory->isAllocated());
  }
  [[nodiscard]] bool isMapped() const { return isAllocated() && _memory->isMapped(); }

  [[nodiscard]] VkImageViewType imageViewType() const;

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 protected:
  void create(const Device& device, const VkImageCreateInfo& imageInfo);

 protected:
  VkImage _image = VK_NULL_HANDLE;

  VkImageType _type             = VK_IMAGE_TYPE_2D;
  VkFormat _format              = VK_FORMAT_UNDEFINED;
  VkExtent3D _extent            = {0, 0, 0};
  mutable VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;

  std::shared_ptr<DeviceMemory> _memory;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)