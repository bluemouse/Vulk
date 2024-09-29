#pragma once

#include <volk/volk.h>

#include <functional>
#include <vector>
#include <limits>
#include <memory>

#include <Vulk/internal/base.h>

#include <Vulk/Image2D.h>
#include <Vulk/ImageView.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class PhysicalDevice;
class Surface;
class RenderPass;

class Swapchain : public Sharable<Swapchain>, private NotCopyable {
 public:
  using SwapchainCreateInfoOverride = std::function<void(VkSwapchainCreateInfoKHR*)>;
  using ChooseSurfaceExtentFunc     = std::function<VkExtent2D(const VkSurfaceCapabilitiesKHR&)>;
  using ChooseSurfaceFormatFunc =
      std::function<VkSurfaceFormatKHR(const std::vector<VkSurfaceFormatKHR>&)>;
  using ChoosePresentModeFunc =
      std::function<VkPresentModeKHR(const std::vector<VkPresentModeKHR>&)>;

 public:
  Swapchain(const Device& device,
            const Surface& surface,
            const VkExtent2D& surfaceExtent,
            const VkSurfaceFormatKHR& surfaceFormat,
            const VkPresentModeKHR& presentMode);
  // TODO: I need to add noexcept for the destructor to avoid the error of "exception specification
  // of overriding function is more lax than base version". Don't know why!!??
  ~Swapchain() noexcept override;

  void create(const Device& device,
              const Surface& surface,
              const VkExtent2D& surfaceExtent,
              const VkSurfaceFormatKHR& surfaceFormat,
              const VkPresentModeKHR& presentMode);
  void create(const Device& device,
              const Surface& surface,
              const ChooseSurfaceExtentFunc& chooseSurfaceExtent,
              const ChooseSurfaceFormatFunc& chooseSurfaceFormat,
              const ChoosePresentModeFunc& choosePresentMode);
  void destroy();

  void resize(uint32_t width, uint32_t height);

  operator VkSwapchainKHR() const { return _swapchain; }

  [[nodiscard]] VkFormat surfaceFormat() const { return _surfaceFormat.format; }
  [[nodiscard]] VkExtent2D surfaceExtent() const { return _surfaceExtent; }

  [[nodiscard]] std::vector<Image2D::shared_ptr>& images() { return _images; }
  [[nodiscard]] Image2D& image(size_t i) { return *_images[i]; }
  [[nodiscard]] std::vector<ImageView::shared_ptr>& imageViews() { return _imageViews; }
  [[nodiscard]] ImageView& imageView(size_t i) { return *_imageViews[i]; }

  [[nodiscard]] const std::vector<Image2D::shared_ptr>& images() const { return _images; }
  [[nodiscard]] const Image2D& image(size_t i) const { return *_images[i]; }
  [[nodiscard]] const std::vector<ImageView::shared_ptr>& imageViews() const { return _imageViews; }
  [[nodiscard]] const ImageView& imageView(size_t i) const { return *_imageViews[i]; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }
  [[nodiscard]] const Surface& surface() const { return *_surface.lock(); }

  [[nodiscard]] bool isCreated() const { return _swapchain != VK_NULL_HANDLE; }

  // Acquire the next image from the swapchain and tag it as the active image. If failed,
  // throw exception.
  void acquireNextImage(const Vulk::Semaphore& signal = {}, const Fence& fence = {}) const;

  // Present the active image to the surface. If failed, throw exception.
  void present(const std::vector<Vulk::Semaphore*>& waits) const;

  [[nodiscard]] uint32_t activeImageIndex() const { return _activeImageIndex; }

  [[nodiscard]] Image& activeImage() { return image(activeImageIndex()); }
  [[nodiscard]] ImageView& activeImageView() { return imageView(activeImageIndex()); }

  [[nodiscard]] const Image& activeImage() const { return image(activeImageIndex()); }
  [[nodiscard]] const ImageView& activeImageView() const { return imageView(activeImageIndex()); }

  [[nodiscard]] VkExtent2D chooseSurfaceExtent(uint32_t windowWidth, uint32_t windowHeight);

 private:
  void deactivateActiveImage() const { _activeImageIndex = std::numeric_limits<uint32_t>::max(); }
  bool hasActiveImage() const { return _activeImageIndex != std::numeric_limits<uint32_t>::max(); }

 private:
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

  VkExtent2D _surfaceExtent{};
  VkSurfaceFormatKHR _surfaceFormat{};

  VkPresentModeKHR _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

  std::vector<Image2D::shared_ptr> _images;
  std::vector<ImageView::shared_ptr> _imageViews;

  mutable uint32_t _activeImageIndex = std::numeric_limits<uint32_t>::max();

  std::weak_ptr<const Device> _device;
  std::weak_ptr<const Surface> _surface;

  mutable bool _requiredRecreate = false;
};

MI_NAMESPACE_END(Vulk)