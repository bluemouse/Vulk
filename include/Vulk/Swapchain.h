#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/Framebuffer.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class PhysicalDevice;
class Surface;
class RenderPass;
class Semaphore;
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
            const VkExtent2D& surfaceExtent,
            const VkSurfaceFormatKHR& surfaceFormat,
            VkPresentModeKHR presentMode);
  ~Swapchain();

  // Disable copy and assignment operators
  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  void create(const Device& device,
              const Surface& surface,
              const VkExtent2D& surfaceExtent,
              const VkSurfaceFormatKHR& surfaceFormat,
              VkPresentModeKHR presentMode);
  void destroy();

  void resize(const VkExtent2D& surfaceExtent);

  void createFramebuffers(const RenderPass& renderPass);
  void resizeFramebuffers(const RenderPass& renderPass);

  operator VkSwapchainKHR() const { return _swapchain; }

  [[nodiscard]] VkFormat surfaceFormat() const { return _surfaceFormat.format; }
  [[nodiscard]] VkExtent2D surfaceExtent() const { return _surfaceExtent; }

  [[nodiscard]] const std::vector<Image>& images() const { return _images; }
  [[nodiscard]] const Image& image(size_t i) const { return _images[i]; }
  [[nodiscard]] const std::vector<ImageView>& imageViews() const { return _imageViews; }
  [[nodiscard]] const ImageView& imageView(size_t i) const { return _imageViews[i]; }
  [[nodiscard]] const std::vector<Framebuffer>& framebuffers() const { return _framebuffers; }
  [[nodiscard]] const Framebuffer& framebuffer(size_t i) const { return _framebuffers[i]; }

  [[nodiscard]] const Device& device() const { return *_device; }

  [[nodiscard]] bool isCreated() const { return _swapchain != VK_NULL_HANDLE; }

  [[nodiscard]] int32_t acquireNextImage(const Vulkan::Semaphore& imageAvailable) const;

  VkResult present(uint32_t imageIndex, const Vulkan::Semaphore& renderFinished) const;

 private:
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

  VkExtent2D _surfaceExtent{};
  VkSurfaceFormatKHR _surfaceFormat{};

  VkPresentModeKHR _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

  std::vector<Image> _images;
  std::vector<ImageView> _imageViews;
  std::vector<Framebuffer> _framebuffers;

  const Device* _device = nullptr;
  const Surface* _surface = nullptr;
};

NAMESPACE_VULKAN_END