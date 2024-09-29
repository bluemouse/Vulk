#include <Vulk/DescriptorSet.h>

#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/Device.h>
#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

DescriptorSet::DescriptorSet(const DescriptorPool& pool, const DescriptorSetLayout& layout) {
  allocate(pool, layout);
}

DescriptorSet::DescriptorSet(const DescriptorPool& pool,
                             const DescriptorSetLayout& layout,
                             const std::vector<Binding>& bindings)
    : DescriptorSet(pool, layout) {
  bind(bindings);
}

DescriptorSet::~DescriptorSet() {
  if (isAllocated()) {
    free();
  }
}

void DescriptorSet::allocate(const DescriptorPool& pool, const DescriptorSetLayout& layout) {
  MI_VERIFY(!isAllocated());

  _layout = layout.get_weak();
  _pool   = pool.get_weak();

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = layout;

  MI_VERIFY_VK_RESULT(vkAllocateDescriptorSets(pool.device(), &allocInfo, &_set));
}

void DescriptorSet::free() {
  MI_VERIFY(isAllocated());

  // TODO: Can only call free() if _pool is created with
  // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
  //  vkFreeDescriptorSets(_pool->device(), *_pool, 1, &_set);

  _set = VK_NULL_HANDLE;
  _pool.reset();
}

void DescriptorSet::bind(const std::vector<Binding>& bindings) const {
  MI_VERIFY(isAllocated());

  const auto& pool   = _pool.lock();
  const auto& layout = _layout.lock();

  const auto& layoutBindings = layout->bindings();
  MI_VERIFY(bindings.size() == layoutBindings.size());

  std::vector<VkWriteDescriptorSet> writes{bindings.size()};

  for (size_t i = 0; i < writes.size(); ++i) {
    const auto& layoutBinding = layoutBindings[i].vkBinding;
    writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet          = *this;
    writes[i].dstBinding      = layoutBinding.binding;
    writes[i].dstArrayElement = 0;
    writes[i].descriptorType  = layoutBinding.descriptorType;
    writes[i].descriptorCount = 1;

    MI_VERIFY(bindings[i].name == layoutBindings[i].name &&
              bindings[i].type == layoutBindings[i].type);
    if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      MI_VERIFY(bindings[i].bufferInfo != nullptr);
      writes[i].pBufferInfo = bindings[i].bufferInfo;
    } else if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
      MI_VERIFY(bindings[i].imageInfo != nullptr);
      writes[i].pImageInfo = bindings[i].imageInfo;
    }

    // We only support these two types for now.
    // TODO: support other types.
    MI_ASSERT(layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
              layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  }

  vkUpdateDescriptorSets(pool->device(), writes.size(), writes.data(), 0, nullptr);
}

MI_NAMESPACE_END(Vulk)
