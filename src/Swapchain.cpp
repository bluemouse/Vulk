#include <Vulk/Swapchain.h>

#include <Vulk/Device.h>
#include <Vulk/Surface.h>
#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Semaphore.h>

#include <limits>

NAMESPACE_BEGIN(Vulk)

Swapchain::Swapchain(const Device& device,
                     const Surface& surface,
                     const VkExtent2D& surfaceExtent,
                     const VkSurfaceFormatKHR& surfaceFormat,
                     VkPresentModeKHR presentMode) {
  create(device, surface, surfaceExtent, surfaceFormat, presentMode);
}

Swapchain::~Swapchain() noexcept {
  if (isCreated()) {
    destroy();
  }
}

void Swapchain::create(const Device& device,
                       const Surface& surface,
                       const VkExtent2D& surfaceExtent,
                       const VkSurfaceFormatKHR& surfaceFormat,
                       VkPresentModeKHR presentMode) {
  MI_VERIFY(!isCreated());
  _device  = device.get_weak();
  _surface = surface.get_weak();

  const auto& physicalDevice = device.physicalDevice();
  const auto capabilities    = surface.querySupports().capabilities;

  _surfaceExtent = surfaceExtent;
  _surfaceFormat = surfaceFormat;
  _presentMode   = presentMode;

  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
    minImageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount    = minImageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  uint32_t queueFamilyIndices[] = {physicalDevice.queueFamilies().graphicsIndex(),
                                   physicalDevice.queueFamilies().presentIndex()};

  if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;       // Optional
    createInfo.pQueueFamilyIndices   = nullptr; // Optional
  }

  createInfo.preTransform   = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode    = _presentMode;
  createInfo.clipped        = VK_TRUE;

  // To assure we use the latest surface extent to create the swapchain, we choose the extent as
  // close to the creation as possible.
  _surfaceExtent         = chooseSurfaceExtent(_surfaceExtent.width, _surfaceExtent.height);
  createInfo.imageExtent = _surfaceExtent;

  MI_VERIFY_VKCMD(vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain));
}

void Swapchain::create(const Device& device,
                       const Surface& surface,
                       const ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                       const ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                       const ChoosePresentModeFunc& choosePresentMode) {
  const auto [capabilities, formats, presentModes] = surface.querySupports();
  const auto extent                                = chooseSurfaceExtent(capabilities);
  const auto format                                = chooseSurfaceFormat(formats);
  const auto presentMode                           = choosePresentMode(presentModes);

  create(device, surface, extent, format, presentMode);
}
void Swapchain::createFramebuffers(const RenderPass& renderPass) {
  MI_VERIFY(isCreated());
  MI_VERIFY(renderPass.colorFormat() == _surfaceFormat.format);

  const auto& device = this->device();

  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr);
  std::vector<VkImage> imgs(imageCount);
  vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, imgs.data());

  _depthImage = DepthImage::make_shared(device, _surfaceExtent, renderPass.depthStencilFormat());
  _depthImage->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  _depthImageView = ImageView::make_shared(device, *_depthImage);

  // We need to reserve the space first to avoid resizing (which triggers the destructor)
  _images.reserve(imageCount);
  _imageViews.reserve(imageCount);
  _framebuffers.reserve(_imageViews.size());
  for (auto& img : imgs) {
    auto img2d = Vulk::Image2D::make_shared(img, renderPass.colorFormat(), _surfaceExtent);
    _images.push_back(img2d);

    auto view = Vulk::ImageView::make_shared(device, *img2d);
    _imageViews.push_back(view);

    auto framebuffer = Vulk::Framebuffer::make_shared(device, renderPass, *view, *_depthImageView);
    _framebuffers.push_back(framebuffer);
  }

  deactivateActiveImage();
}

void Swapchain::destroy() {
  MI_VERIFY(isCreated());

  deactivateActiveImage();

  // Be careful about changing the destroying order.
  _framebuffers.clear();
  _imageViews.clear();
  _images.clear();
  _depthImageView.reset();
  _depthImage.reset();

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);

  _surfaceExtent = {};
  _surfaceFormat = {};
  _presentMode   = VK_PRESENT_MODE_IMMEDIATE_KHR;

  _swapchain = VK_NULL_HANDLE;
  _device.reset();
  _surface.reset();
}

void Swapchain::resize(uint32_t width, uint32_t height) {
  MI_VERIFY(isCreated());

  if (width == _surfaceExtent.width && height == _surfaceExtent.height) {
    return;
  }
  RenderPass::shared_const_ptr renderPass;
  if (!_framebuffers.empty()) {
    renderPass = _framebuffers[0]->renderPass().get_shared();

    // Be careful about changing the destroying order.
    _framebuffers.clear();
    _imageViews.clear();
    _images.clear();

    _depthImageView.reset();
    _depthImage.reset();
  }

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);
  _swapchain = VK_NULL_HANDLE;

  const auto surfaceExtent = chooseSurfaceExtent(width, height);

  create(device(), surface(), surfaceExtent, _surfaceFormat, _presentMode);

  if (renderPass) {
    createFramebuffers(*renderPass);
  }
}

VkResult Swapchain::acquireNextImage(const Vulk::Semaphore& imageAvailable) const {
  deactivateActiveImage();

  auto result = vkAcquireNextImageKHR(device(),
                                      _swapchain,
                                      std::numeric_limits<uint64_t>::max(),
                                      imageAvailable,
                                      VK_NULL_HANDLE,
                                      &_activeImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return VK_ERROR_OUT_OF_DATE_KHR; // Require a swapchain resize
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Error: failed to acquire swapchain image!");
  }

  return result;
}

VkResult Swapchain::present(const Vulk::Semaphore& renderFinished) const {
  MI_VERIFY(hasActiveImage());

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = renderFinished;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains    = &_swapchain;

  presentInfo.pImageIndices = &_activeImageIndex;

  return vkQueuePresentKHR(device().queue("present"), &presentInfo);
}

VkExtent2D Swapchain::chooseSurfaceExtent(uint32_t windowWidth, uint32_t windowHeight) {
  const auto caps = surface().querySupports().capabilities;

  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  } else {
    auto width  = std::clamp(windowWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
    auto height = std::clamp(windowHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    return {width, height};
  }
}

NAMESPACE_END(Vulk)
