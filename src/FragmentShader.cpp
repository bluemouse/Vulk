#include <Vulk/FragmentShader.h>

#include <Vulk/Device.h>

NAMESPACE_Vulk_BEGIN

FragmentShader::FragmentShader(FragmentShader&& rhs) noexcept {
  moveFrom(rhs);
}

FragmentShader& FragmentShader::operator=(FragmentShader&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void FragmentShader::moveFrom(FragmentShader& rhs) {
  MI_VERIFY(!isCreated());
  ShaderModule::moveFrom(rhs);
}

void FragmentShader::addDescriptorSetLayoutBinding(const std::string& name,
                                                   const std::string& type,
                                                   uint32_t binding,
                                                   VkDescriptorType descriptorType) {
  ShaderModule::addDescriptorSetLayoutBinding(
      name, type, binding, descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);
}

NAMESPACE_Vulk_END
