#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Image.h>
#include <Vulk/internal/vulkan_debug.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class DepthImage : public Image {
 public:
  using ImageCreateInfoOverride = std::function<void(VkImageCreateInfo*)>;

 public:
  DepthImage() = default;
  DepthImage(const Device& device,
             VkExtent2D extent,
             uint32_t depthBits,
             uint32_t stencilBits                    = 0,
             const ImageCreateInfoOverride& override = {});

  // Transfer the ownership from `rhs` to `this`
  DepthImage(DepthImage&& rhs) noexcept;
  DepthImage& operator=(DepthImage&& rhs) noexcept(false);

  void create(const Device& device,
              VkExtent2D extent,
              uint32_t depthBits,
              uint32_t stencilBits                    = 0,
              const ImageCreateInfoOverride& override = {});
  void create(const Device& device,
              VkExtent2D extent,
              VkFormat format,
              const ImageCreateInfoOverride& override = {});

  using Image::destroy;
  using Image::allocate;
  using Image::free;

  void promoteLayout(const CommandBuffer& cmdBuffer, bool waitForFinish);

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  using Image::isCreated;
  using Image::isAllocated;

  operator VkImage() const { return Image::operator VkImage(); }

  [[nodiscard]] bool hasDepthBits() const;
  [[nodiscard]] bool hasStencilBits() const;

  [[nodiscard]] static VkFormat findFormat(uint32_t depthBits, uint32_t stencilBits);

 private:
  VkFormat _format = VK_FORMAT_UNDEFINED;
};

NAMESPACE_END(Vulk)