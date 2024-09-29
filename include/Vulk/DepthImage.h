#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#include <Vulk/Image.h>

MI_NAMESPACE_BEGIN(Vulk)

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
  DepthImage(const Device& device,
             VkExtent2D extent,
             VkFormat format,
             const ImageCreateInfoOverride& override = {});

  ~DepthImage() override = default;

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

  void promoteLayout(const Queue& queue, const CommandBuffer& cmdBuffer, bool waitForFinish = true);

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  using Image::isCreated;
  using Image::isAllocated;

  operator VkImage() const { return Image::operator VkImage(); }

  [[nodiscard]] bool hasDepthBits() const;
  [[nodiscard]] bool hasStencilBits() const;

  [[nodiscard]] static VkFormat findFormat(uint32_t depthBits, uint32_t stencilBits);

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(DepthImage, Image);

 private:
  VkFormat _format = VK_FORMAT_UNDEFINED;
};

MI_NAMESPACE_END(Vulk)
