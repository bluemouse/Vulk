#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class Image;
class Image2D;
class DepthImage;

class ImageView : public Sharable<ImageView>, private NotCopyable {
 public:
  using ImageViewCreateInfoOverride = std::function<void(VkImageViewCreateInfo&)>;

 public:
  ImageView() = default;
  ImageView(const Device& device,
            Image2D& image,
            ImageViewCreateInfoOverride createInfoOverride = {});
  ImageView(const Device& device,
            DepthImage& image,
            ImageViewCreateInfoOverride createInfoOverride = {});
  ~ImageView();

  void create(const Device& device,
              Image2D& image,
              ImageViewCreateInfoOverride createInfoOverride = {});
  void create(const Device& device,
              DepthImage& image,
              ImageViewCreateInfoOverride createInfoOverride = {});
  void destroy();

  operator VkImageView() const { return _view; }

  [[nodiscard]] Image& image() { return *_image.lock(); }
  [[nodiscard]] const Image& image() const { return *_image.lock(); }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  [[nodiscard]] bool isCreated() const { return _view != VK_NULL_HANDLE; }

 private:
  void create(const Device& device,
              Image& image,
              VkImageAspectFlags aspectMask,
              ImageViewCreateInfoOverride createInfoOverride);

 private:
  VkImageView _view = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
  std::weak_ptr<Image> _image;
};

NAMESPACE_END(Vulk)