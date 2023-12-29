#include <Vulk/FragmentShader.h>

NAMESPACE_BEGIN(Vulk)

void FragmentShader::addDescriptorSetLayoutBinding(const std::string& name,
                                                   const std::string& type,
                                                   uint32_t binding,
                                                   VkDescriptorType descriptorType) {
  ShaderModule::addDescriptorSetLayoutBinding(
      name, type, binding, descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);
}

NAMESPACE_END(Vulk)
