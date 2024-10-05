#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <Vulk/Instance.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <set>
#include <string>

#include <Vulk/internal/base.h>
#include <Vulk/internal/debug.h>
#include <Vulk/internal/helpers.h>

#include <Vulk/Surface.h>

namespace {
struct Initializer {
  Initializer() { volkInitialize(); }
};
static Initializer init;
}

MI_NAMESPACE_BEGIN(Vulk)

Instance::Instance(
    const ApplicationInfoOverride& appInfoOverride,
    const InstanceCreateInfoOverride& instanceCreateInfoOverride,
    const DebugUtilsMessengerCreateInfoOverride& debugUtilsMessengerCreateInfoOverride) {
  create(appInfoOverride, instanceCreateInfoOverride, debugUtilsMessengerCreateInfoOverride);
}

Instance::Instance(int versionMajor,
                   int versionMinor,
                   std::vector<const char*> extensions,
                   ValidationLevel validation) {
  create(versionMajor, versionMinor, std::move(extensions), validation);
}

Instance::~Instance() {
  if (isCreated()) {
    destroy();
  }
}

void Instance::create(
    const ApplicationInfoOverride& appInfoOverride,
    const InstanceCreateInfoOverride& instanceCreateInfoOverride,
    const DebugUtilsMessengerCreateInfoOverride& debugUtilsMessengerCreateInfoOverride) {
  MI_VERIFY(!isCreated());

  if (debugUtilsMessengerCreateInfoOverride) {
    _layers = {"VK_LAYER_KHRONOS_validation"};
    MI_VERIFY(checkLayerSupport(_layers));
  }

  initDefaultValidationCallback();

  VkApplicationInfo appInfo{};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = nullptr;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = nullptr;
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_MAKE_API_VERSION(0, 1, 0, 0);

  if (appInfoOverride) {
    appInfoOverride(&appInfo);
  }

  VkInstanceCreateInfo createInfo{};
  createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext                   = nullptr;
  createInfo.pApplicationInfo        = &appInfo;
  createInfo.enabledLayerCount       = 0;
  createInfo.enabledExtensionCount   = 0;
  createInfo.ppEnabledExtensionNames = nullptr;

  if (instanceCreateInfoOverride) {
    instanceCreateInfoOverride(&createInfo);
  }

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (debugUtilsMessengerCreateInfoOverride) {
    debugUtilsMessengerCreateInfoOverride(&debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  }

  MI_VERIFY_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &_instance));
  volkLoadInstanceOnly(_instance);

  if (debugUtilsMessengerCreateInfoOverride) {
    MI_VERIFY_VK_RESULT(
        vkCreateDebugUtilsMessengerEXT(_instance, &debugCreateInfo, nullptr, &_debugMessenger));
  }
}

void Instance::create(int versionMajor,
                      int versionMinor,
                      std::vector<const char*> extensions,
                      ValidationLevel validation) {
  MI_VERIFY(!isCreated());

  auto appInfoOverride = [versionMajor, versionMinor](VkApplicationInfo* appInfo) {
    appInfo->apiVersion = VK_MAKE_API_VERSION(0, versionMajor, versionMinor, 0);
  };

  DebugUtilsMessengerCreateInfoOverride debugUtilsMessengerCreateInfoOverride;
  if (validation != ValidationLevel::None) {
    debugUtilsMessengerCreateInfoOverride = [&](VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
      createInfo->sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      createInfo->messageSeverity = 0x00;
      if (validation >= ValidationLevel::Error) {
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      }
      if (validation >= ValidationLevel::Warning) {
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
      }
      if (validation >= ValidationLevel::Info) {
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
      }
      if (validation >= ValidationLevel::Verbose) {
        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
      }
      createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      createInfo->pfnUserCallback = Instance::VkDebugCallback;
      createInfo->pUserData       = this;
    };
  }

  auto instanceCreateInfoOverride = [&](VkInstanceCreateInfo* createInfo) {
    if (validation != ValidationLevel::None) {
      createInfo->enabledLayerCount   = static_cast<uint32_t>(_layers.size());
      createInfo->ppEnabledLayerNames = _layers.data();

      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    createInfo->enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo->ppEnabledExtensionNames = extensions.data();
  };

  create(appInfoOverride, instanceCreateInfoOverride, debugUtilsMessengerCreateInfoOverride);
}

void Instance::destroy() {
  MI_VERIFY(isCreated());

  _physicalDevice = nullptr;

  if (_debugMessenger != VK_NULL_HANDLE) {
    vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
  }

  vkDestroyInstance(_instance, nullptr);

  _instance       = VK_NULL_HANDLE;
  _debugMessenger = VK_NULL_HANDLE;
}

void Instance::pickPhysicalDevice(const Surface& surface,
                                  const PhysicalDevice::QueueFamilies& queueFamilies,
                                  const std::vector<const char*>& deviceExtensions,
                                  const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures) {
  _physicalDevice = PhysicalDevice::make_shared(
      *this, &surface, queueFamilies, deviceExtensions, hasDeviceFeatures);
  _physicalDevice->initQueueFamilies(surface);
}

void Instance::pickPhysicalDevice(const PhysicalDevice::QueueFamilies& queueFamilies,
                                  const std::vector<const char*>& deviceExtensions,
                                  const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures) {
  _physicalDevice = PhysicalDevice::make_shared(
      *this, nullptr, queueFamilies, deviceExtensions, hasDeviceFeatures);
  _physicalDevice->initQueueFamilies();
}

bool Instance::checkLayerSupport(const std::vector<const char*>& layers) {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers{layerCount};
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  std::set<std::string> availableLayerNames;
  for (const auto& layer : availableLayers) {
    availableLayerNames.insert(layer.layerName);
  }

  for (const auto* layer : layers) {
    if (availableLayerNames.find(layer) == availableLayerNames.end()) {
      return false;
    }
  }

  return true;
}

void Instance::initDefaultValidationCallback() {
  _validationCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT* data) -> VkBool32 {
    std::string severity;
    switch (messageSeverity) {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "VERBOSE"; break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "INFO"; break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity = "\033[36mWARNING\033[0m"; // cyan
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity = "\033[1;31mERROR\033[0m"; // bold red
        break;
      default: severity = "UNKNOWN"; break;
    }
    std::string type;
    switch (messageType) {
      case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "GENERAL"; break;
      case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        type = "\033[33mVALIDATION\033[0m"; // yellow
        break;
      case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        type = "\033[1;32mPERFORMANCE\033[0m"; // bold gree
        break;
      default: type = "UNKNOWN"; break;
    }

    std::string message = std::string{"Vulkan ["} + type + " " + severity + "]: " + data->pMessage;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      std::cerr << message << std::endl;
      // throw std::runtime_error(message);
    } else {
      std::cerr << message << std::endl;
    }
    return VK_FALSE;
  };
}

void Instance::setValidationCallback(const ValidationCallback& callback) {
  if (callback) {
    _validationCallback = callback;
  } else {
    initDefaultValidationCallback();
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
Instance::VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                          void* pUserData) {
  auto* instance = static_cast<Instance*>(pUserData);
  return instance->_validationCallback(messageSeverity, messageType, pCallbackData);
}

MI_NAMESPACE_END(Vulk)
