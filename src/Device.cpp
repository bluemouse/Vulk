#include <Vulk/Device.h>

#include <set>
#include <utility>

#include <Vulk/internal/debug.h>

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>

NAMESPACE_BEGIN(Vulk)

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

void Device::create(const PhysicalDevice& physicalDevice,
                    const std::vector<uint32_t>& queueFamilies,
                    const std::vector<const char*>& extensions,
                    const DeviceCreateInfoOverride& override) {
  MI_VERIFY(!isCreated());
  _physicalDevice = physicalDevice.get_weak();

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0F;
  std::set<uint32_t> uniqueQueueFamilies{queueFamilies.begin(), queueFamilies.end()};
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos    = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  const auto& instance = physicalDevice.instance();
  if (instance.isValidationLayersEnabled()) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(instance.layers().size());
    createInfo.ppEnabledLayerNames = instance.layers().data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (override) {
    override(&createInfo, &deviceFeatures, &queueCreateInfos);
  }

  MI_VERIFY_VKCMD(vkCreateDevice(physicalDevice, &createInfo, nullptr, &_device));
}

void Device::initQueue(QueueFamilyName queueFamilyName, uint32_t queueFamilyIndex) {
  MI_VERIFY(isCreated());
  VkQueue queue = VK_NULL_HANDLE;
  vkGetDeviceQueue(_device, queueFamilyIndex, 0, &queue);
  _queues[queueFamilyName] = {queueFamilyIndex, queue};
}

VkQueue Device::queue(QueueFamilyName queueFamilyName) const {
  MI_VERIFY(isCreated());
  if (_queues.find(queueFamilyName) == _queues.end()) {
    return VK_NULL_HANDLE;
  }
  return _queues.at(queueFamilyName).queue;
}

std::optional<uint32_t> Device::queueIndex(QueueFamilyName queueFamilyName) const {
  MI_VERIFY(isCreated());
  if (_queues.find(queueFamilyName) == _queues.end()) {
    return std::nullopt;
  }
  return _queues.at(queueFamilyName).index;
}

std::vector<uint32_t> Device::queueIndices() const {
  MI_VERIFY(isCreated());
  std::set<uint32_t> indices;
  for (const auto& [_, queue] : _queues) {
    indices.insert(queue.index);
  }
  return {indices.begin(), indices.end()};
}

void Device::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyDevice(_device, nullptr);

  _device = VK_NULL_HANDLE;
  _queues.clear();
  _physicalDevice.reset();
}

void Device::waitIdle() const {
  MI_VERIFY(isCreated());
  vkDeviceWaitIdle(_device);
}

const Instance& Device::instance() const {
  return physicalDevice().instance();
}

NAMESPACE_END(Vulk)
