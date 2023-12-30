#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include <Vulk/internal/base.h>

struct SpvReflectShaderModule;

NAMESPACE_BEGIN(Vulk)

class Device;

class ShaderModule : public Sharable<ShaderModule>, private NotCopyable {
 public:
  struct DescriptorSetLayoutBinding {
    std::string name;
    std::string type;
    VkDescriptorSetLayoutBinding vkBinding{};

    bool operator==(const DescriptorSetLayoutBinding& rhs) const {
      return name == rhs.name && type == rhs.type && vkBinding.binding == rhs.vkBinding.binding &&
             vkBinding.descriptorType == rhs.vkBinding.descriptorType &&
             vkBinding.descriptorCount == rhs.vkBinding.descriptorCount &&
             vkBinding.stageFlags == rhs.vkBinding.stageFlags;
    }
  };
  struct VertexInputAttribute {
    std::string name;
    std::string type;
    VkVertexInputAttributeDescription vkDescription{};
  };

 public:
  // If `reflection` is true, SPIRV-Reflect is used to generate VkDescriptorSetLayoutBinding,
  // VkVertexInputBindingDescription (vertex shader only) and VkVertexInputAttributeDescription
  // (vertex shader only) from the shader code reflection.
  //
  // All input attributes are assumed to be bound to binding 0. I.e. We assume there is one vertex
  // buffer to supply all input attributes. Also, all vertex attributes are assumed to be per-vertex
  // only.
  ShaderModule(const Device& device, const std::vector<char>& codes, bool reflection = true);
  ShaderModule(const Device& device, const char* shaderFile, bool reflection = true);
  virtual ~ShaderModule() override;

  void create(const Device& device, const std::vector<char>& codes, bool reflection = true);
  void create(const Device& device, const char* shaderFile, bool reflection = true);
  void destroy();

  operator VkShaderModule() const { return _shader; }

  [[nodiscard]] bool isCreated() const { return _shader != VK_NULL_HANDLE; }

  void addDescriptorSetLayoutBinding(const std::string& name,
                                     const std::string& type,
                                     uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     VkShaderStageFlags stageFlags);

  [[nodiscard]] const std::vector<DescriptorSetLayoutBinding>& descriptorSetLayoutBindings() const {
    return _descriptorSetLayoutBindings;
  }

  void setEntry(const char* entry) { _entry = entry; }
  [[nodiscard]] const char* entry() const { return _entry.c_str(); }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  static void enablePrintReflection();
  static void disablePrintReflection();

 private:
  void reflectShader(const std::vector<char>& codes);
  void reflectDescriptorSets(const SpvReflectShaderModule& module);
  void reflectVertexInputs(const SpvReflectShaderModule& module);

 protected:
  VkShaderModule _shader = VK_NULL_HANDLE;

  std::string _entry{};

  std::vector<DescriptorSetLayoutBinding> _descriptorSetLayoutBindings;
  std::vector<VertexInputAttribute> _vertexInputAttributes;
  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)