#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

#include <Vulk/Framebuffer.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class PhysicalDevice;
class Surface;
class RenderPass;
class Swapchain {
 public:
  using SwapchainCreateInfoOverride = std::function<void(VkSwapchainCreateInfoKHR*)>;
  using ChooseSurfaceFormat =
      std::function<VkSurfaceFormatKHR(const std::vector<VkSurfaceFormatKHR>&)>;
  using ChoosePresentMode = std::function<VkPresentModeKHR(const std::vector<VkPresentModeKHR>&)>;
  using ChooseExtent = std::function<VkExtent2D(const VkSurfaceCapabilitiesKHR&)>;

 public:
  Swapchain() = default;
  Swapchain(const Device& device,
            const Surface& surface,
            const ChooseSurfaceFormat& chooseSurfaceFormat,
            const ChoosePresentMode& choosePresentMode,
            const ChooseExtent& chooseSwapExtent);
  ~Swapchain();

  // Disable copy and assignment operators
  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  void create(const Device& device,
              const Surface& surface,
              const ChooseSurfaceFormat& chooseSurfaceFormat,
              const ChoosePresentMode& choosePresentMode,
              const ChooseExtent& chooseSwapExtent);
  void createFramebuffers(const RenderPass& renderPass);
  void destroy();

  operator VkSwapchainKHR() const { return _swapchain; }

  [[nodiscard]] const std::vector<Image>& images() const { return _images; }
  [[nodiscard]] VkFormat imageFormat() const { return _images[0].format(); }
  [[nodiscard]] VkExtent2D imageExtent() const { return {_images[0].width(), _images[0].height()}; }

  [[nodiscard]] const std::vector<ImageView>& imageViews() const { return _imageViews; }
  [[nodiscard]] const ImageView& imageView(size_t i) const { return _imageViews[i]; }

  [[nodiscard]] const std::vector<Framebuffer>& framebuffers() const { return _framebuffers; }
  [[nodiscard]] const Framebuffer& framebuffer(size_t i) const { return _framebuffers[i]; }

  [[nodiscard]] const Device& device() const { return *_device; }

  [[nodiscard]] bool isCreated() const { return _swapchain != VK_NULL_HANDLE; }

 private:
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

  std::vector<Image> _images;
  std::vector<ImageView> _imageViews;
  std::vector<Framebuffer> _framebuffers;

  const Device* _device = nullptr;
};

NAMESPACE_VULKAN_END