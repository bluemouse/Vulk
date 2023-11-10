#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Image.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

    class Device;

class DepthImage : protected Image {
 public:
  using ImageCreateInfoOverride = std::function<void(VkImageCreateInfo*)>;

 public:
  DepthImage() = default;
  DepthImage(const Device& device,
             VkFormat format,
             VkExtent2D extent,
             const ImageCreateInfoOverride& override = {});
  DepthImage(const Device& device,
             VkFormat format,
             VkExtent2D extent,
             VkMemoryPropertyFlags properties,
             const ImageCreateInfoOverride& override = {});

  // Transfer the ownership from `rhs` to `this`
  DepthImage(DepthImage&& rhs) noexcept;
  DepthImage& operator=(DepthImage&& rhs) noexcept(false);

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              const ImageCreateInfoOverride& override = {});

  using Image::destroy;
  using Image::allocate;
  using Image::free;

  void promoteLayout(const CommandBuffer& cmdBuffer, bool waitForFinish);

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  using Image::isCreated;
  using Image::isAllocated;

  operator VkImage() const { return Image::operator VkImage(); }
};

NAMESPACE_Vulk_END