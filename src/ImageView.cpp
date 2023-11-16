#include <Vulk/ImageView.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/Image2D.h>
#include <Vulk/DepthImage.h>

NAMESPACE_BEGIN(Vulk)

ImageView::ImageView(const Device& device,
                     const Image2D& image,
                     ImageViewCreateInfoOverride createInfoOverride) {
  create(device, image, createInfoOverride);
}
ImageView::ImageView(const Device& device,
                     const DepthImage& image,
                     ImageViewCreateInfoOverride createInfoOverride) {
  create(device, image, createInfoOverride);
}

ImageView::~ImageView() {
  if (isCreated()) {
    destroy();
  }
}

ImageView::ImageView(ImageView&& rhs) noexcept {
  moveFrom(rhs);
}

ImageView& ImageView::operator=(ImageView&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void ImageView::moveFrom(ImageView& rhs) {
  MI_VERIFY(!isCreated());
  _view   = rhs._view;
  _device = rhs._device;
  _image  = rhs._image;

  rhs._view   = VK_NULL_HANDLE;
  rhs._device = nullptr;
  rhs._image  = nullptr;
}

void ImageView::create(const Device& device,
                       const Image2D& image,
                       ImageViewCreateInfoOverride createInfoOverride) {
  auto aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  create(device, image, aspect, createInfoOverride);
}

void ImageView::create(const Device& device,
                       const DepthImage& image,
                       ImageViewCreateInfoOverride createInfoOverride) {
  auto aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  create(device, image, aspect, createInfoOverride);
}

void ImageView::create(const Device& device,
                       const Image& image,
                       VkImageAspectFlags aspectMask,
                       ImageViewCreateInfoOverride createInfoOverride) {
  MI_VERIFY(!isCreated());
  _device = &device;
  _image  = &image;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = image.imageViewType();
  viewInfo.format                          = image.format();
  viewInfo.subresourceRange.aspectMask     = aspectMask;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if (createInfoOverride) {
    createInfoOverride(viewInfo);
  }

  MI_VERIFY_VKCMD(vkCreateImageView(*_device, &viewInfo, nullptr, &_view));
}

void ImageView::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyImageView(*_device, _view, nullptr);

  _view   = VK_NULL_HANDLE;
  _device = nullptr;
  _image  = nullptr;
}

NAMESPACE_END(Vulk)
