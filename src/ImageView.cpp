#include <Vulk/ImageView.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/Image2D.h>
#include <Vulk/DepthImage.h>

MI_NAMESPACE_BEGIN(Vulk)

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

void ImageView::create(const Device& device,
                       const Image2D& image,
                       ImageViewCreateInfoOverride createInfoOverride) {
  auto aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  create(device, image, aspect, createInfoOverride);
}

void ImageView::create(const Device& device,
                       const DepthImage& image,
                       ImageViewCreateInfoOverride createInfoOverride) {
  VkImageAspectFlags aspect = 0;
  if (image.hasDepthBits()) {
    aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  if (image.hasStencilBits()) {
    aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  create(device, image, aspect, createInfoOverride);
}

void ImageView::create(const Device& device,
                       const Image& image,
                       VkImageAspectFlags aspectMask,
                       ImageViewCreateInfoOverride createInfoOverride) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();
  _image  = image.get_weak();

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

  MI_VERIFY_VK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &_view));
}

void ImageView::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyImageView(device(), _view, nullptr);

  _view = VK_NULL_HANDLE;
  _device.reset();
  _image.reset();
}

MI_NAMESPACE_END(Vulk)
