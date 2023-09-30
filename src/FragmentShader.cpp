#include <Vulk/FragmentShader.h>

#include <Vulk/Device.h>

NAMESPACE_VULKAN_BEGIN

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

void FragmentShader::addDescriptorSetLayoutBinding(uint32_t binding,
                                                   VkDescriptorType type) {
  ShaderModule::addDescriptorSetLayoutBinding(binding, type, VK_SHADER_STAGE_FRAGMENT_BIT);
}

NAMESPACE_VULKAN_END
