#include <Vulk/Swapchain.h>

#include <Vulk/Device.h>
#include <Vulk/Surface.h>
#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Semaphore.h>

#include <limits>

NAMESPACE_Vulk_BEGIN

Swapchain::Swapchain(const Device& device,
                     const Surface& surface,
                     const VkExtent2D& surfaceExtent,
                     const VkSurfaceFormatKHR& surfaceFormat,
                     VkPresentModeKHR presentMode) {
  create(device, surface, surfaceExtent, surfaceFormat, presentMode);
}

Swapchain::~Swapchain() {
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
  _device = &device;
  _surface = &surface;

  const auto& physicalDevice = device.physicalDevice();
  const auto capabilities = surface.querySupports().capabilities;

  _surfaceExtent = surfaceExtent;
  _surfaceFormat = surfaceFormat;
  _presentMode = presentMode;

  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
    minImageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = _surfaceExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {physicalDevice.queueFamilies().graphicsIndex(),
                                   physicalDevice.queueFamilies().presentIndex()};

  if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;     // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = _presentMode;
  createInfo.clipped = VK_TRUE;

  MI_VERIFY_VKCMD(vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain));
}

void Swapchain::create(const Device& device,
                       const Surface& surface,
                       const ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                       const ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                       const ChoosePresentModeFunc& choosePresentMode) {
  const auto [capabilities, formats, presentModes] = surface.querySupports();
  const auto extent = chooseSurfaceExtent(capabilities);
  const auto format = chooseSurfaceFormat(formats);
  const auto presentMode = choosePresentMode(presentModes);

  create(device, surface, extent, format, presentMode);
}
void Swapchain::createFramebuffers(const RenderPass& renderPass) {
  MI_VERIFY(isCreated());

  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(*_device, _swapchain, &imageCount, nullptr);
  std::vector<VkImage> imgs(imageCount);
  vkGetSwapchainImagesKHR(*_device, _swapchain, &imageCount, imgs.data());

  // We need to reserve the space first to avoid resizing (which triggers the destructor)
  _images.reserve(imageCount);
  _imageViews.reserve(imageCount);
  _framebuffers.reserve(_imageViews.size());
  for (auto& img : imgs) {
    _images.emplace_back(img, _surfaceFormat.format, _surfaceExtent);
    _imageViews.emplace_back(*_device, _images.back());
    _framebuffers.emplace_back(*_device, renderPass, _imageViews.back());
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

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);

  _surfaceExtent = {};
  _surfaceFormat = {};
  _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

  _swapchain = VK_NULL_HANDLE;
  _device = nullptr;
  _surface = nullptr;
}

void Swapchain::resize(const VkExtent2D& surfaceExtent) {
  MI_VERIFY(isCreated());

  if (surfaceExtent.width == _surfaceExtent.width &&
      surfaceExtent.height == _surfaceExtent.height) {
    return;
  }

  const RenderPass* renderPass = nullptr;
  if (!_framebuffers.empty()) {
    renderPass = _framebuffers[0].renderPass();

    // Be careful about changing the destroying order.
    _framebuffers.clear();
    _imageViews.clear();
    _images.clear();
  }

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);
  _swapchain = VK_NULL_HANDLE;

  create(*_device, *_surface, surfaceExtent, _surfaceFormat, _presentMode);

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
  presentInfo.pWaitSemaphores = renderFinished;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &_swapchain;

  presentInfo.pImageIndices = &_activeImageIndex;

  return vkQueuePresentKHR(_device->queue("present"), &presentInfo);
}

NAMESPACE_Vulk_END
