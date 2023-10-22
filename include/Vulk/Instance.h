#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

#include <Vulk/PhysicalDevice.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

class Surface;

class Instance {
 public:
  using ApplicationInfoOverride = std::function<void(VkApplicationInfo*)>;
  using InstanceCreateInfoOverride = std::function<void(VkInstanceCreateInfo*)>;
  using DebugUtilsMessengerCreateInfoOverride =
      std::function<void(VkDebugUtilsMessengerCreateInfoEXT*)>;

  Instance() = default;
  Instance(const ApplicationInfoOverride& appInfoOverride,
           const InstanceCreateInfoOverride& instanceCreateInfoOverride,
           const DebugUtilsMessengerCreateInfoOverride& debugUtilsMessengerCreateInfoOverride = {});
  Instance(int versionMajor,
           int versionMinor,
           std::vector<const char*> extensions,
           bool enableValidation = false);
  ~Instance();

  void create(const ApplicationInfoOverride& appInfoOverride = {},
              const InstanceCreateInfoOverride& instanceCreateInfoOverride = {},
              const DebugUtilsMessengerCreateInfoOverride& messengerCreateInfoOverride = {});
  void create(int versionMajor,
              int versionMinor,
              std::vector<const char*> extensions,
              bool enableValidation = false);

  void destroy();

  void pickPhysicalDevice(const Surface& surface,
                          const PhysicalDevice::IsDeviceSuitableFunc& isDeviceSuitable);

  operator VkInstance() const { return _instance; }
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return _physicalDevice; }

  [[nodiscard]] bool isValidationLayersEnabled() const { return !_layers.empty(); }
  [[nodiscard]] const std::vector<const char*>& layers() const { return _layers; }

  using ValidationCallback = std::function<VkBool32(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                    VkDebugUtilsMessageTypeFlagsEXT,
                                                    const VkDebugUtilsMessengerCallbackDataEXT*)>;
  void setValidationCallback(const ValidationCallback& callback);

  [[nodiscard]] bool isCreated() const { return _instance != VK_NULL_HANDLE; }

  // Disable copy and assignment operators
  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

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
  PhysicalDevice _physicalDevice;

  VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

  std::vector<const char*> _layers;

  ValidationCallback _validationCallback;
};

NAMESPACE_Vulk_END