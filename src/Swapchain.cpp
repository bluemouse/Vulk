#include <Vulk/Swapchain.h>

#include <limits>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/Surface.h>
#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>
#include <Vulk/Queue.h>
#include <Vulk/Exception.h>

NAMESPACE_BEGIN(Vulk)

Swapchain::Swapchain(const Device& device,
                     const Surface& surface,
                     const VkExtent2D& surfaceExtent,
                     const VkSurfaceFormatKHR& surfaceFormat,
                     const VkPresentModeKHR& presentMode) {
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
                       const VkPresentModeKHR& presentMode) {
  MI_VERIFY(!isCreated());
  _device  = device.get_weak();
  _surface = surface.get_weak();

  const auto capabilities = surface.querySupports().capabilities;

  _surfaceExtent = surfaceExtent;
  _surfaceFormat = surfaceFormat;
  _presentMode   = presentMode;

  constexpr uint32_t PREFERRED_IMAGE_COUNT = 3; // Triple buffering
  uint32_t minImageCount = std::min(std::max(PREFERRED_IMAGE_COUNT, capabilities.minImageCount),
                                    capabilities.maxImageCount);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount    = minImageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  auto queueFamilyIndices = device.queueFamilyIndices();
  if (queueFamilyIndices.size() == 1) {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;       // Optional
    createInfo.pQueueFamilyIndices   = nullptr; // Optional
  } else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
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

  // Initialize the swapchain images.
  std::vector<VkImage> imgs{minImageCount};
  MI_VERIFY_VKCMD(vkGetSwapchainImagesKHR(device, _swapchain, &minImageCount, imgs.data()));

  // We need to reserve the space first to avoid resizing (which triggers the destructor)
  _images.reserve(minImageCount);
  _imageViews.reserve(minImageCount);
  for (auto& img : imgs) {
    auto img2d = Vulk::Image2D::make_shared(img, _surfaceFormat.format, _surfaceExtent);
    _images.push_back(img2d);

    auto imgView = Vulk::ImageView::make_shared(device, *img2d);
    _imageViews.push_back(imgView);
  }

  deactivateActiveImage();

  _requiredRecreate = false;
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

void Swapchain::destroy() {
  MI_VERIFY(isCreated());

  deactivateActiveImage();

  // Be careful about changing the destroying order.
  _imageViews.clear();
  _images.clear();

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
  _imageViews.clear();
  _images.clear();

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);
  _swapchain = VK_NULL_HANDLE;

  const auto surfaceExtent = chooseSurfaceExtent(width, height);
  create(device(), surface(), surfaceExtent, _surfaceFormat, _presentMode);
}

void Swapchain::acquireNextImage(const Semaphore& signal, const Fence& fence) const {
  auto result = vkAcquireNextImageKHR(device(),
                                      _swapchain,
                                      std::numeric_limits<uint64_t>::max(),
                                      signal,
                                      fence,
                                      &_activeImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    _requiredRecreate = true;
  }
  if (result != VK_SUCCESS) {
    throw Exception{result, "Failed to acquire swapchain image."};
  }
}

void Swapchain::present(const std::vector<Semaphore*>& waits) const {
  MI_VERIFY(hasActiveImage());

  std::vector<VkSemaphore> semaphores;
  for (auto wait : waits) {
    semaphores.push_back(*wait);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = semaphores.size();
  presentInfo.pWaitSemaphores    = semaphores.data();

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains    = &_swapchain;

  presentInfo.pImageIndices = &_activeImageIndex;

  auto result = vkQueuePresentKHR(device().queue(Device::QueueFamilyType::Present), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    _requiredRecreate = true;
  }
  if (result != VK_SUCCESS) {
    throw Exception{result, "Failed to present swapchain image."};
  }
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
