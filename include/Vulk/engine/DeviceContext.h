#pragma once

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/Surface.h>
#include <Vulk/Swapchain.h>
#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

class DeviceContext : public Sharable<DeviceContext>, private NotCopyable {
 public:
  using ValidationLevel         = Instance::ValidationLevel;
  using CreateWindowSurfaceFunc = std::function<VkSurfaceKHR(const Instance& instance)>;

 public:
  struct CreateInfo {
    // The info in CreateInfo is used to create the Vulkan context and listed in the usage order.
    int versionMajor = 1;
    int versionMinor = 0;
    std::vector<const char*> instanceExtensions;
    ValidationLevel validationLevel = ValidationLevel::None;

    CreateWindowSurfaceFunc createWindowSurface;

    PhysicalDevice::QueueFamilies queueFamilies; // To specify the queue families to be created
    std::vector<const char*> deviceExtensions;   // To specify the required device extensions
    PhysicalDevice::HasDeviceFeaturesFunc hasPhysicalDeviceFeatures;

    Swapchain::ChooseSurfaceFormatFunc chooseSurfaceFormat;
    Swapchain::ChooseSurfaceExtentFunc chooseSurfaceExtent;
    Swapchain::ChoosePresentModeFunc choosePresentMode;
  };

 public:
  DeviceContext()          = default;
  virtual ~DeviceContext() = default;

  virtual void create(const CreateInfo& createInfo);
  virtual void destroy();

  void waitIdle() const;

  [[nodiscard]] bool isComplete() const;

  [[nodiscard]] Instance& instance() { return *_instance; }
  [[nodiscard]] Surface& surface() { return *_surface; }
  [[nodiscard]] Device& device() { return *_device; }
  [[nodiscard]] Swapchain& swapchain() { return *_swapchain; }
  [[nodiscard]] Queue& queue(Device::QueueFamilyType queueFamily);
  [[nodiscard]] CommandPool& commandPool(Device::QueueFamilyType queueFamily);

  [[nodiscard]] const Instance& instance() const { return *_instance; }
  [[nodiscard]] const Surface& surface() const { return *_surface; }
  [[nodiscard]] const Device& device() const { return *_device; }
  [[nodiscard]] const Swapchain& swapchain() const { return *_swapchain; }
  [[nodiscard]] const Queue& queue(Device::QueueFamilyType queueFamily) const;
  [[nodiscard]] const CommandPool& commandPool(Device::QueueFamilyType queueFamily) const;

  [[nodiscard]] bool isQueueFamilySupported(Device::QueueFamilyType queueFamily) const;

 protected:
  virtual void createInstance(int versionMajor,
                              int versionMinor,
                              const std::vector<const char*>& extensions,
                              ValidationLevel validation = ValidationLevel::None);
  virtual void createSurface(const CreateWindowSurfaceFunc& createWindowSurface);
  virtual void pickPhysicalDevice(const PhysicalDevice::QueueFamilies& queueFamilies,
                                  const std::vector<const char*>& deviceExtensions,
                                  const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures);
  virtual void createDevice(const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
                            const std::vector<const char*>& deviceExtensions);
  virtual void createSwapchain(const Swapchain::ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                               const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                               const Swapchain::ChoosePresentModeFunc& choosePresentMode);

 protected:
  Instance::shared_ptr _instance;

  Surface::shared_ptr _surface;
  Device::shared_ptr _device;
  Swapchain::shared_ptr _swapchain;

  PhysicalDevice::QueueFamilies _queueFamilies;
};

MI_NAMESPACE_END(Vulk)
