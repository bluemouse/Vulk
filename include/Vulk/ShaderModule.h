#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

#include <vector>

struct SpvReflectShaderModule;
struct SpvReflectShaderModule;

NAMESPACE_Vulk_BEGIN

    class Device;

class ShaderModule {
 public:
  ShaderModule() = default;
  // If `reflection` is true, SPIRV-Reflect is used to generate VkDescriptorSetLayoutBinding,
  // VkVertexInputBindingDescription (vertex shader only) and VkVertexInputAttributeDescription
  // (vertex shader only) from the shader code reflection.
  //
  // All input attributes are assumed to be bound to binding 0. I.e. We assume there is one vertex
  // buffer to supply all input attributes. Also, all vertex attributes are assumed to be per-vertex
  // only.
  ShaderModule(const Device& device, const std::vector<char>& codes, bool reflection = true);
  ShaderModule(const Device& device, const char* shaderFile, bool reflection = true);
  virtual ~ShaderModule();

  void create(const Device& device, const std::vector<char>& codes, bool reflection = true);
  void create(const Device& device, const char* shaderFile, bool reflection = true);
  void destroy();

  // Transfer the ownership from `rhs` to `this`
  ShaderModule(ShaderModule&& rhs) noexcept;
  ShaderModule& operator=(ShaderModule&& rhs) noexcept(false);

  operator VkShaderModule() const { return _shader; }

  [[nodiscard]] bool isCreated() const { return _shader != VK_NULL_HANDLE; }

  void addDescriptorSetLayoutBinding(uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     VkShaderStageFlags stageFlags);

  [[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings()
      const {
    return _descriptorSetLayoutBindings;
  }

  void setEntry(const char* entry) { _entry = entry; }
  [[nodiscard]] const char* entry() const { return _entry.c_str(); }

 protected:
  void moveFrom(ShaderModule& rhs);

 private:
  void reflectShader(const std::vector<char>& codes);
  void reflectDescriptorSets(const SpvReflectShaderModule& module);
  void reflectVertexInputs(const SpvReflectShaderModule& module);

 protected:
  VkShaderModule _shader = VK_NULL_HANDLE;

  std::string _entry{};

  struct DescriptorSetLayoutBindingBindingMeta {
    std::string name;
    std::string typeName;
  };
  std::vector<DescriptorSetLayoutBindingBindingMeta> _descriptorSetLayoutBindingsMeta;
  std::vector<VkDescriptorSetLayoutBinding> _descriptorSetLayoutBindings;

  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  struct VertexInputAttributeMeta {
    std::string name;
    std::string type;
  };
  std::vector<VertexInputAttributeMeta> _vertexInputAttributesMeta;
  std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;

  const Device* _device;
};

NAMESPACE_Vulk_END