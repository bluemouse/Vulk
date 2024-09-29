#include <Vulk/DescriptorSetLayout.h>

#include <Vulk/Device.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/internal/debug.h>

#include <map>
#include <vector>
#include <algorithm>

MI_NAMESPACE_BEGIN(Vulk)

DescriptorSetLayout::DescriptorSetLayout(const Device& device, std::vector<ShaderModule*> shaders) {
  create(device, std::move(shaders));
}

DescriptorSetLayout::~DescriptorSetLayout() {
  if (isCreated()) {
    destroy();
  }
}

void DescriptorSetLayout::create(const Device& device, std::vector<ShaderModule*> shaders) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  size_t numBindings = 0;
  for (auto* shader : shaders) {
    numBindings += shader->descriptorSetLayoutBindings().size();
  }
  _bindings.reserve(numBindings);

  // Append all bindings from all shaders to `_bindings`
  for (auto* shader : shaders) {
    _bindings.insert(std::end(_bindings),
                     std::begin(shader->descriptorSetLayoutBindings()),
                     std::end(shader->descriptorSetLayoutBindings()));
  }

  // Sort the bindings in the order the binding number
  std::sort(std::begin(_bindings), std::end(_bindings), [](const auto& lhs, const auto& rhs) {
    return lhs.vkBinding.binding < rhs.vkBinding.binding;
  });

  // Make sure there is no duplicate bindings
  std::unique(std::begin(_bindings), std::end(_bindings));

  // TODO Should we verify that there is no mutiple types in one the bindings?

  std::map<VkDescriptorType, int> typeCounts;
  for (const auto& layoutBinding : _bindings) {
    typeCounts[layoutBinding.vkBinding.descriptorType] += 1;
  }

  _poolSizes.reserve(typeCounts.size());
  for (const auto& [type, count] : typeCounts) {
    _poolSizes.push_back({type, static_cast<uint32_t>(count)});
  }

  std::vector<VkDescriptorSetLayoutBinding> vkBindings;
  vkBindings.reserve(_bindings.size());
  for (const auto& layoutBinding : _bindings) {
    vkBindings.push_back(layoutBinding.vkBinding);
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
  layoutInfo.pBindings    = vkBindings.data();

  MI_VERIFY_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout));
}

void DescriptorSetLayout::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyDescriptorSetLayout(device(), _layout, nullptr);
  _layout = VK_NULL_HANDLE;
  _bindings.clear();
  _poolSizes.clear();
  _device.reset();
}

MI_NAMESPACE_END(Vulk)
