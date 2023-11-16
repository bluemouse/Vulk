#include <Vulk/DescriptorSet.h>

#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/Device.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

DescriptorSet::DescriptorSet(const DescriptorPool& pool,
                             const DescriptorSetLayout& layout,
                             const std::vector<Binding>& bindings) {
  allocate(pool, layout, bindings);
}

DescriptorSet::~DescriptorSet() {
  if (isAllocated()) {
    free();
  }
}

DescriptorSet::DescriptorSet(DescriptorSet&& rhs) noexcept {
  moveFrom(rhs);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void DescriptorSet::moveFrom(DescriptorSet& rhs) {
  MI_VERIFY(!isAllocated());
  _set  = rhs._set;
  _pool = rhs._pool;

  rhs._set  = VK_NULL_HANDLE;
  rhs._pool = nullptr;
}

void DescriptorSet::allocate(const DescriptorPool& pool,
                             const DescriptorSetLayout& layout,
                             const std::vector<Binding>& bindings) {
  MI_VERIFY(!isAllocated());
  _pool = &pool;

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = layout;

  MI_VERIFY_VKCMD(vkAllocateDescriptorSets(pool.device(), &allocInfo, &_set));

  const auto& layoutBindings = layout.bindings();
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

  vkUpdateDescriptorSets(_pool->device(), writes.size(), writes.data(), 0, nullptr);
}

void DescriptorSet::free() {
  MI_VERIFY(isAllocated());

  // TODO: Can only call free() if _pool is created with
  // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
  //  vkFreeDescriptorSets(_pool->device(), *_pool, 1, &_set);

  _set  = VK_NULL_HANDLE;
  _pool = nullptr;
}

NAMESPACE_END(Vulk)
