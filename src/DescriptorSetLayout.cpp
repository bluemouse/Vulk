#include <Vulk/DescriptorSetLayout.h>

#include <Vulk/Device.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/helpers_vulkan.h>

#include <map>

NAMESPACE_Vulk_BEGIN

DescriptorSetLayout::DescriptorSetLayout(const Device& device, std::vector<ShaderModule*> shaders) {
  create(device, std::move(shaders));
}

DescriptorSetLayout::~DescriptorSetLayout() {
  if (isCreated()) {
    destroy();
  }
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& rhs) noexcept {
  moveFrom(rhs);
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void DescriptorSetLayout::moveFrom(DescriptorSetLayout& rhs) {
  MI_VERIFY(!isCreated());
  _layout = rhs._layout;
  _bindings = std::move(rhs._bindings);
  _poolSizes = std::move(rhs._poolSizes);
  _device = rhs._device;

  rhs._layout = VK_NULL_HANDLE;
  rhs._bindings.clear();
  rhs._poolSizes.clear();
  rhs._device = nullptr;
}

void DescriptorSetLayout::create(const Device& device, std::vector<ShaderModule*> shaders) {
  MI_VERIFY(!isCreated());
  _device = &device;

  size_t numBindings = 0;
  for (auto* shader : shaders) {
    numBindings += shader->descriptorSetLayoutBindings().size();
  }

  std::map<VkDescriptorType, int> types;
  _bindings.reserve(numBindings);
  for (auto* shader : shaders) {
    for (const auto& binding : shader->descriptorSetLayoutBindings()) {
      _bindings.push_back(binding);
      types[binding.descriptorType] += 1;
    }
  }

  _poolSizes.reserve(types.size());
  for (const auto& [type, count] : types) {
    _poolSizes.push_back({type, static_cast<uint32_t>(count)});
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(_bindings.size());
  layoutInfo.pBindings = _bindings.data();

  MI_VERIFY_VKCMD(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout));
}

void DescriptorSetLayout::destroy() {
  MI_VERIFY(isCreated());

  vkDestroyDescriptorSetLayout(*_device, _layout, nullptr);
  _layout = VK_NULL_HANDLE;
  _bindings.clear();
  _poolSizes.clear();
  _device = nullptr;
}

NAMESPACE_Vulk_END
