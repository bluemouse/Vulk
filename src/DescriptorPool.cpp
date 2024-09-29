#include <Vulk/DescriptorPool.h>

#include <Vulk/internal/base.h>
#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/DescriptorSetLayout.h>

#include <utility>

MI_NAMESPACE_BEGIN(Vulk)

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

void DescriptorPool::create(const Device& device,
                            std::vector<VkDescriptorPoolSize> poolSizes,
                            uint32_t maxSets) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  for (auto& poolSize : poolSizes) {
    poolSize.descriptorCount *= maxSets;
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = maxSets;

  MI_VERIFY_VK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &_pool));
}

void DescriptorPool::create(const DescriptorSetLayout& layout, uint32_t maxSets) {
  create(layout.device(), layout.poolSizes(), maxSets);
}

void DescriptorPool::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyDescriptorPool(device(), _pool, nullptr);

  _pool = VK_NULL_HANDLE;
  _device.reset();
}

void DescriptorPool::reset() {
  MI_VERIFY_VK_RESULT(vkResetDescriptorPool(device(), _pool, 0));
}

MI_NAMESPACE_END(Vulk)