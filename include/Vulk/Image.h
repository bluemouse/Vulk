#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/DeviceMemory.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;
class StagingBuffer;

class Image {
 public:
  Image() = default;
  virtual ~Image() noexcept(false);

  // Transfer the ownership from `rhs` to `this`
  Image(Image&& rhs) noexcept;
  Image& operator=(Image&& rhs) noexcept(false);

  virtual void destroy();

  virtual void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  virtual void free();

  virtual void bind(const DeviceMemory::Ptr& memory, VkDeviceSize offset = 0);

  virtual void* map();
  virtual void* map(VkDeviceSize offset, VkDeviceSize size);
  virtual void unmap();

  virtual void copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer);

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

 protected:
  void create(const Device& device, const VkImageCreateInfo& imageInfo);

  void transitToNewLayout(const CommandBuffer& commandBuffer,
                          VkImageLayout newLayout,
                          bool waitForFinish = true) const;

 private:
  void moveFrom(Image& rhs);

 protected:
  VkImage _image = VK_NULL_HANDLE;

  VkImageType _type             = VK_IMAGE_TYPE_2D;
  VkFormat _format              = VK_FORMAT_UNDEFINED;
  VkExtent3D _extent            = {0, 0, 0};
  mutable VkImageLayout _layout = VK_IMAGE_LAYOUT_UNDEFINED;

  DeviceMemory::Ptr _memory;

  const Device* _device = nullptr;
};

NAMESPACE_END(Vulk)