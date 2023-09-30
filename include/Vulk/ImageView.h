#pragma once

#include <vulkan/vulkan.h>

#include <functional>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class Image;

class ImageView {
 public:
  using ImageViewCreateInfoOverride = std::function<void(VkImageViewCreateInfo&)>;

 public:
  ImageView() = default;
  ImageView(const Device& device,
            const Image& image,
            ImageViewCreateInfoOverride createInfoOverride = {});
  ~ImageView();

  // Transfer the ownership from `rhs` to `this`
  ImageView(ImageView&& rhs) noexcept;
  ImageView& operator=(ImageView&& rhs) noexcept(false);

  void create(const Device& device,
              const Image& image,
              ImageViewCreateInfoOverride createInfoOverride = {});
  void destroy();

  operator VkImageView() const { return _view; }

  [[nodiscard]] const Image& image() const { return *_image; }

  [[nodiscard]] bool isCreated() const { return _view != VK_NULL_HANDLE; }

 private:
  void moveFrom(ImageView& rhs);

 private:
  VkImageView _view = VK_NULL_HANDLE;

  const Device* _device = nullptr;
  const Image* _image = nullptr;
};

NAMESPACE_VULKAN_END