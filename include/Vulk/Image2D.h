#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Image.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class Image2D : public Image {
 public:
  using ImageCreateInfoOverride = std::function<void(VkImageCreateInfo*)>;

 public:
  Image2D() = default;
  Image2D(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          const ImageCreateInfoOverride& override = {});
  Image2D(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          VkMemoryPropertyFlags properties,
          const ImageCreateInfoOverride& override = {});

  Image2D(VkImage image, VkFormat format, VkExtent2D extent); // special use by Swapchain

  // Transfer the ownership from `rhs` to `this`
  Image2D(Image2D&& rhs) noexcept;
  Image2D& operator=(Image2D&& rhs) noexcept(false);

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              const ImageCreateInfoOverride& override = {});
  void destroy() override;

  void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) override;
  void free() override;

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  operator VkImage() const { return Image::operator VkImage(); }

 protected:
  bool isExternal() const { return _external; }

 private:
  bool _external = false;
};

NAMESPACE_END(Vulk)