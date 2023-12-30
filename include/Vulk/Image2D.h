#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Vulk/internal/base.h>

#include <Vulk/Image.h>

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
          VkImageUsageFlags usage,
          const ImageCreateInfoOverride& override = {});
  Image2D(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          VkImageUsageFlags usage,
          VkMemoryPropertyFlags properties,
          const ImageCreateInfoOverride& override = {});

  Image2D(VkImage image, VkFormat format, VkExtent2D extent); // special use by Swapchain

  ~Image2D() override;

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              VkImageUsageFlags usage,
              const ImageCreateInfoOverride& override = {});
  void destroy() override;

  void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) override;
  void free() override;

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  operator VkImage() const { return Image::operator VkImage(); }

  //
  // Override the sharable types and functions
  //
  using shared_ptr = std::shared_ptr<Image2D>;
  using weak_ptr   = std::weak_ptr<Image2D>;

  template <class... Args>
  static shared_ptr make_shared(Args&&... args) {
    return std::make_shared<Image2D>(std::forward<Args>(args)...);
  }

  shared_ptr get_shared() { return std::static_pointer_cast<Image2D>(Image::get_shared()); }
  weak_ptr get_weak() { return std::static_pointer_cast<Image2D>(Image::get_weak().lock()); }

 protected:
  bool isExternal() const { return _external; }

 private:
  bool _external = false;
};

NAMESPACE_END(Vulk)