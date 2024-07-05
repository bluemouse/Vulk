#include <Vulk/Device.h>

#include <set>
#include <utility>

#include <Vulk/internal/debug.h>

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/Queue.h>
#include <Vulk/CommandPool.h>

NAMESPACE_BEGIN(Vulk)

Device::Device(const PhysicalDevice& physicalDevice,
               const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
               const std::vector<const char*>& extensions,
               const DeviceCreateInfoOverride& override) {
  create(physicalDevice, requiredQueueFamilies, extensions, override);
}

Device::~Device() {
  if (isCreated()) {
    destroy();
  }
}

void Device::create(const PhysicalDevice& physicalDevice,
                    const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
                    const std::vector<const char*>& extensions,
                    const DeviceCreateInfoOverride& override) {
  MI_VERIFY(!isCreated());
  _physicalDevice = physicalDevice.get_weak();

  const auto& supportedQueueFamilies = physicalDevice.queueFamilies();

  // Extract the queue family indices and types.
  if (requiredQueueFamilies.graphics) {
    MI_VERIFY_MSG(supportedQueueFamilies.graphics,
                  "Required graphics queue family is not supported by the physical device.");
    _queueFamilies.push_back({QueueFamilyType::Graphics, supportedQueueFamilies.graphicsIndex()});
  }
  if (requiredQueueFamilies.compute) {
    MI_VERIFY_MSG(supportedQueueFamilies.compute,
                  "Required compute queue family is not supported by the physical device.");
    _queueFamilies.push_back({QueueFamilyType::Compute, supportedQueueFamilies.computeIndex()});
  }
  if (requiredQueueFamilies.transfer) {
    MI_VERIFY_MSG(supportedQueueFamilies.transfer,
                  "Required transfer queue family is not supported by the physical device.");
    _queueFamilies.push_back({QueueFamilyType::Transfer, supportedQueueFamilies.transferIndex()});
  }
  if (requiredQueueFamilies.present) {
    MI_VERIFY_MSG(supportedQueueFamilies.present,
                  "Required present queue family is not supported by the physical device.");
    _queueFamilies.push_back({QueueFamilyType::Present, supportedQueueFamilies.presentIndex()});
  }

  // Create the device.
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0F;
  std::set<uint32_t> uniqueQueueFamilies;
  for (const auto& queueFamily : _queueFamilies) {
    uniqueQueueFamilies.insert(queueFamily.index);
  }
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount       = 1; // TODO For now we only allow one queue per family.
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfo.flags            = 0; // We have to set it to 0 for now.
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

void Device::initQueues() {
  MI_VERIFY(isCreated());
  for (const auto& queueFamily : _queueFamilies) {
    _queues[queueFamily.type] = std::make_shared<Queue>(*this, queueFamily.type, queueFamily.index);
  }
}

void Device::initCommandPools() {
  MI_VERIFY(isCreated());
  for (const auto& queueFamily : _queueFamilies) {
    _commandPools[queueFamily.type] = std::make_shared<CommandPool>(*this, queueFamily.type);
  }
}

Queue& Device::queue(QueueFamilyType queueFamilyType) {
  MI_VERIFY(isCreated());
  MI_VERIFY(_queues[queueFamilyType]);
  return *_queues[queueFamilyType];
}

const Queue& Device::queue(QueueFamilyType queueFamilyType) const {
  MI_VERIFY(isCreated());
  MI_VERIFY(_queues[queueFamilyType]);
  return *_queues[queueFamilyType];
}

std::optional<uint32_t> Device::queueFamilyIndex(QueueFamilyType queueFamilyType) const {
  MI_VERIFY(isCreated());
  if (!_queues[queueFamilyType]) {
    return std::nullopt;
  }
  return _queues[queueFamilyType]->queueFamilyIndex();
}

std::vector<uint32_t> Device::queueFamilyIndices() const {
  MI_VERIFY(isCreated());
  std::set<uint32_t> indices;
  for (const auto& queue : _queues) {
    if (queue) {
      indices.insert(queue->queueFamilyIndex());
    }
  }
  return {indices.begin(), indices.end()};
}

CommandPool& Device::commandPool(QueueFamilyType queueFamilyType) {
  MI_VERIFY(isCreated());
  MI_VERIFY(_commandPools[queueFamilyType]);
  return *_commandPools[queueFamilyType];
}

const CommandPool& Device::commandPool(QueueFamilyType queueFamilyType) const {
  MI_VERIFY(isCreated());
  MI_VERIFY(_commandPools[queueFamilyType]);
  return *_commandPools[queueFamilyType];
}


void Device::destroy() {
  MI_VERIFY(isCreated());

  _commandPools.clear();
  _queues.clear();

  vkDestroyDevice(_device, nullptr);

  _device = VK_NULL_HANDLE;
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
