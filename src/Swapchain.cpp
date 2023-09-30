#include <Vulk/Swapchain.h>

#include <Vulk/Device.h>
#include <Vulk/Surface.h>
#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/RenderPass.h>

NAMESPACE_VULKAN_BEGIN

Swapchain::Swapchain(const Device& device,
                     const Surface& surface,
                     const ChooseSurfaceFormat& chooseSurfaceFormat,
                     const ChoosePresentMode& choosePresentMode,
                     const ChooseExtent& chooseExtent) {
  create(device, surface, chooseSurfaceFormat, choosePresentMode, chooseExtent);
}

Swapchain::~Swapchain() {
  if (isCreated()) {
    destroy();
  }
}

void Swapchain::create(const Device& device,
                       const Surface& surface,
                       const ChooseSurfaceFormat& chooseSurfaceFormat,
                       const ChoosePresentMode& choosePresentMode,
                       const ChooseExtent& chooseExtent) {
  MI_VERIFY(!isCreated());
  _device = &device;

  const auto& physicalDevice = device.physicalDevice();
  const auto [capabilities, formats, presentModes] = surface.querySupports();

  VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
  VkPresentModeKHR presentMode = choosePresentMode(presentModes);
  VkExtent2D extent = chooseExtent(capabilities);

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
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
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  MI_VERIFY_VKCMD(vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain));

  vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr);
  std::vector<VkImage> imgs(imageCount);
  vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, imgs.data());

  // We need to reserve the space first to avoid resizing (which triggers the destructor)
  _images.reserve(imageCount);
  for (auto& img : imgs) {
    _images.emplace_back(img, surfaceFormat.format, extent);
  }

  // We need to reserve the space first to avoid resizing (which triggers the destructor)
  _imageViews.reserve(imageCount);
  for (auto& image : _images) {
    _imageViews.emplace_back(device, image);
  }
}

void Swapchain::createFramebuffers(const RenderPass& renderPass) {
  MI_VERIFY(isCreated());

  // Reserve enough space to avoid resizing that can trigger destructing
  _framebuffers.reserve(_imageViews.size());
  for (auto& view : _imageViews) {
    _framebuffers.emplace_back(device(), renderPass, view);
  }
}

void Swapchain::destroy() {
  MI_VERIFY(isCreated());

  // Be careful about changing the release order.
  _framebuffers.clear();
  _imageViews.clear();
  _images.clear();

  vkDestroySwapchainKHR(device(), _swapchain, nullptr);

  _swapchain = VK_NULL_HANDLE;
  _device = nullptr;
}

NAMESPACE_VULKAN_END
