#pragma once

#include <volk/volk.h>

#include <memory>

#include <Vulk/internal/base.h>
#include <Vulk/internal/helpers.h>

#include <Vulk/Image.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;

class Image2D : public Image {
 public:
  enum Usage : uint8_t {
    NONE                     = 0x00,
    TRANSFER_SRC             = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    TRANSFER_DST             = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    SAMPLED                  = VK_IMAGE_USAGE_SAMPLED_BIT, // Image2D is always sampled
    STORAGE                  = VK_IMAGE_USAGE_STORAGE_BIT,
    COLOR_ATTACHMENT         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    DEPTH_STENCIL_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
  };

  using ImageCreateInfoOverride = std::function<void(VkImageCreateInfo*)>;

 public:
  Image2D() = default;
  Image2D(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          Usage usage                             = Usage::NONE,
          const ImageCreateInfoOverride& override = {});
  Image2D(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          Usage usage,
          VkMemoryPropertyFlags properties,
          const ImageCreateInfoOverride& override = {});

  // Special use by Swapchain
  Image2D(VkImage image,
          VkFormat format,
          VkExtent2D extent,
          VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED);

  ~Image2D() override;

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              Usage usage                             = Usage::NONE,
              const ImageCreateInfoOverride& override = {});
  void destroy() override;

  void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) override;
  void free() override;

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  operator VkImage() const { return Image::operator VkImage(); }

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(Image2D, Image);

 protected:
  bool isExternal() const { return _external; }

 private:
  bool _external = false;
};

MI_ENABLE_ENUM_BITWISE_OP(Image2D::Usage);

MI_NAMESPACE_END(Vulk)