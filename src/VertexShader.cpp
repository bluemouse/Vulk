#include <Vulk/VertexShader.h>

#include <Vulk/Device.h>

NAMESPACE_BEGIN(Vulk)

VertexShader::VertexShader(VertexShader&& rhs) noexcept {
  moveFrom(rhs);
}

VertexShader& VertexShader::operator=(VertexShader&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void VertexShader::moveFrom(VertexShader& rhs) {
  MI_VERIFY(!isCreated());
  ShaderModule::moveFrom(rhs);

  _vertexInputBindings   = std::move(rhs._vertexInputBindings);
  _vertexInputAttributes = std::move(rhs._vertexInputAttributes);

  rhs._vertexInputBindings.clear();
  rhs._vertexInputAttributes.clear();
}

void VertexShader::addVertexInputBinding(uint32_t binding,
                                         uint32_t stride,
                                         VkVertexInputRate inputRate) {
  _vertexInputBindings.push_back({binding, stride, inputRate});
}

void VertexShader::addVertexInputBindings(std::vector<VkVertexInputBindingDescription> bindings) {
  _vertexInputBindings.insert(_vertexInputBindings.end(), bindings.begin(), bindings.end());
}

void VertexShader::addVertexInputAttribute(const std::string& name,
                                           const std::string& type,
                                           uint32_t location,
                                           uint32_t binding,
                                           VkFormat format,
                                           uint32_t offset) {
  _vertexInputAttributes.push_back({name, type, {location, binding, format, offset}});
}

void VertexShader::addVertexInputAttributes(std::vector<VertexInputAttribute> attributes) {
  _vertexInputAttributes.insert(_vertexInputAttributes.end(), attributes.begin(), attributes.end());
}

void VertexShader::addDescriptorSetLayoutBinding(const std::string& name,
                                                 const std::string& type,
                                                 uint32_t binding,
                                                 VkDescriptorType descriptorType) {
  ShaderModule::addDescriptorSetLayoutBinding(
      name, type, binding, descriptorType, VK_SHADER_STAGE_VERTEX_BIT);
}

NAMESPACE_END(Vulk)
