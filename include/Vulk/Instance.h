#pragma once

#include <volk/volk.h>

#include <functional>
#include <vector>

#include <Vulk/internal/base.h>

#include <Vulk/PhysicalDevice.h>

NAMESPACE_BEGIN(Vulk)

class Surface;

class Instance : public Sharable<Instance>, private NotCopyable {
 public:
  enum class ValidationLevel {
    None    = 0,
    Error   = 1,
    Warning = 2,
    Info    = 3,
    Verbose = 4,
  };

 public:
  using ApplicationInfoOverride    = std::function<void(VkApplicationInfo*)>;
  using InstanceCreateInfoOverride = std::function<void(VkInstanceCreateInfo*)>;
  using DebugUtilsMessengerCreateInfoOverride =
      std::function<void(VkDebugUtilsMessengerCreateInfoEXT*)>;

  explicit Instance(const ApplicationInfoOverride& appInfoOverride = {},
                    const InstanceCreateInfoOverride& instanceCreateInfoOverride = {},
                    const DebugUtilsMessengerCreateInfoOverride& debugUtilsMessengerCreateInfoOverride = {});
  Instance(int versionMajor,
           int versionMinor,
           std::vector<const char*> extensions,
           ValidationLevel validation = ValidationLevel::None);
  ~Instance() override;

  void create(const ApplicationInfoOverride& appInfoOverride                           = {},
              const InstanceCreateInfoOverride& instanceCreateInfoOverride             = {},
              const DebugUtilsMessengerCreateInfoOverride& messengerCreateInfoOverride = {});
  void create(int versionMajor,
              int versionMinor,
              std::vector<const char*> extensions,
              ValidationLevel validation = ValidationLevel::None);

  void destroy();

  void pickPhysicalDevice(const Surface& surface,
                          const PhysicalDevice::QueueFamilies& queueFamilies,
                          const std::vector<const char*>& deviceExtensions,
                          const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures);
  void pickPhysicalDevice(const PhysicalDevice::QueueFamilies& queueFamilies,
                          const std::vector<const char*>& deviceExtensions,
                          const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures);

  operator VkInstance() const { return _instance; }
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return *_physicalDevice; }

  [[nodiscard]] bool isValidationLayersEnabled() const { return !_layers.empty(); }
  [[nodiscard]] const std::vector<const char*>& layers() const { return _layers; }

  using ValidationCallback = std::function<VkBool32(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                    VkDebugUtilsMessageTypeFlagsEXT,
                                                    const VkDebugUtilsMessengerCallbackDataEXT*)>;
  void setValidationCallback(const ValidationCallback& callback);

  [[nodiscard]] bool isCreated() const { return _instance != VK_NULL_HANDLE; }

 private:
  static bool checkLayerSupport(const std::vector<const char*>& layers);
  void initDefaultValidationCallback();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData);

 private:
  VkInstance _instance = VK_NULL_HANDLE;
  std::shared_ptr<PhysicalDevice> _physicalDevice;

  VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

  std::vector<const char*> _layers;

  ValidationCallback _validationCallback;
};

NAMESPACE_END(Vulk)