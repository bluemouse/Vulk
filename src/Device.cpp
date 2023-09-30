#include <Vulk/Device.h>

#include <set>
#include <utility>

#include <Vulk/Instance.h>

NAMESPACE_VULKAN_BEGIN

Device::Device(const PhysicalDevice& physicalDevice,
               const std::vector<uint32_t>& queueFamilies,
               const std::vector<const char*>& extensions,
               const DeviceCreateInfoOverride& override) {
  create(physicalDevice, queueFamilies, extensions, override);
}

Device::~Device() {
  if (isCreated()) {
    destroy();
  }
}

Device::Device(Device&& rhs) noexcept {
  moveFrom(rhs);
}

Device& Device::operator=(Device&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void Device::moveFrom(Device& rhs) {
  MI_VERIFY(!isCreated());
  _device = rhs._device;
  _queues = std::move(rhs._queues);
  _physicalDevice = rhs._physicalDevice;

  rhs._device = VK_NULL_HANDLE;
  rhs._physicalDevice = nullptr;
}

void Device::create(const PhysicalDevice& physicalDevice,
                    const std::vector<uint32_t>& queueFamilies,
                    const std::vector<const char*>& extensions,
                    const DeviceCreateInfoOverride& override) {
  MI_VERIFY(!isCreated());
  _physicalDevice = &physicalDevice;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0F;
  std::set<uint32_t> uniqueQueueFamilies{queueFamilies.begin(), queueFamilies.end()};
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  const auto& instance = physicalDevice.instance();
  if (instance.isValidationLayersEnabled()) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(instance.layers().size());
    createInfo.ppEnabledLayerNames = instance.layers().data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (override) {
    override(&createInfo, &deviceFeatures, &queueCreateInfos);
  }

  MI_VERIFY_VKCMD(vkCreateDevice(physicalDevice, &createInfo, nullptr, &_device));
}

void Device::initQueue(std::string queueName, uint32_t queueFamilyIndex) {
  MI_VERIFY(isCreated());
  VkQueue queue = VK_NULL_HANDLE;
  vkGetDeviceQueue(_device, queueFamilyIndex, 0, &queue);
  _queues[queueName] = queue;
}

VkQueue Device::queue(const std::string& queueName) const {
  MI_VERIFY(isCreated());
  return _queues.at(queueName);
}

void Device::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyDevice(_device, nullptr);

  _device = VK_NULL_HANDLE;
  _queues.clear();
  _physicalDevice = nullptr;
}

void Device::waitIdle() const {
  MI_VERIFY(isCreated());
  vkDeviceWaitIdle(_device);
}
NAMESPACE_VULKAN_END
