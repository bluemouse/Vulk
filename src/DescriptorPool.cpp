#include <Vulk/DescriptorPool.h>

#include <utility>

#include <Vulk/Device.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

DescriptorPool::DescriptorPool(const DescriptorSetLayout& layout, uint32_t maxSets) {
  create(layout, maxSets);
}

DescriptorPool::DescriptorPool(const Device& device,
                               std::vector<VkDescriptorPoolSize> poolSizes,
                               uint32_t maxSets) {
  create(device, std::move(poolSizes), maxSets);
}

DescriptorPool::~DescriptorPool() {
  if (isCreated()) {
    destroy();
  }
}

DescriptorPool::DescriptorPool(DescriptorPool&& rhs) noexcept {
  moveFrom(rhs);
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void DescriptorPool::moveFrom(DescriptorPool& rhs) {
  MI_VERIFY(!isCreated());
  _pool = rhs._pool;
  _device = rhs._device;

  rhs._pool = VK_NULL_HANDLE;
  rhs._device = nullptr;
}

void DescriptorPool::create(const Device& device,
                            std::vector<VkDescriptorPoolSize> poolSizes,
                            uint32_t maxSets) {
  MI_VERIFY(!isCreated());
  _device = &device;

  for (auto& poolSize : poolSizes) {
    poolSize.descriptorCount *= maxSets;
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = maxSets;

  MI_VERIFY_VKCMD(vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool));
}

void DescriptorPool::create(const DescriptorSetLayout& layout, uint32_t maxSets) {
  create(layout.device(), layout.poolSizes(), maxSets);
}

void DescriptorPool::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyDescriptorPool(*_device, _pool, nullptr);
  _pool = VK_NULL_HANDLE;
  _device = nullptr;
}

NAMESPACE_Vulk_END