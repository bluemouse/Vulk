#pragma once

#include <volk/volk.h>

#include <vector>
#include <memory>

#include <Vulk/internal/base.h>

#include <Vulk/ShaderModule.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class VertexShader;
class FragmentShader;
class ComputeShader;

class DescriptorSetLayout : public Sharable<DescriptorSetLayout>, private NotCopyable {
 public:
  using DescriptorSetLayoutBinding = ShaderModule::DescriptorSetLayoutBinding;

 public:
  DescriptorSetLayout() = default;
  DescriptorSetLayout(const Device& device, std::vector<const ShaderModule*> shaders);
  DescriptorSetLayout(const Device& device, const VertexShader& vertShader, const FragmentShader& fragShader);
  DescriptorSetLayout(const Device& device, const ComputeShader& compShader);
  ~DescriptorSetLayout();

  void create(const Device& device, std::vector<const ShaderModule*> shaders);
  void create(const Device& device, const VertexShader& vertShader, const FragmentShader& fragShader);
  void create(const Device& device, const ComputeShader& compShader);
  void destroy();

  operator VkDescriptorSetLayout() const { return _layout; }
  operator const VkDescriptorSetLayout*() const { return &_layout; }

  [[nodiscard]] bool isCreated() const { return _layout != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  [[nodiscard]] const std::vector<VkDescriptorPoolSize>& poolSizes() const { return _poolSizes; }
  [[nodiscard]] const std::vector<DescriptorSetLayoutBinding>& bindings() const {
    return _bindings;
  }

 private:
  VkDescriptorSetLayout _layout = VK_NULL_HANDLE;

  std::vector<DescriptorSetLayoutBinding> _bindings;
  std::vector<VkDescriptorPoolSize> _poolSizes;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)