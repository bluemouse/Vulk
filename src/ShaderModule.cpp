#include <Vulk/ShaderModule.h>

#include <fstream>
#include <vector>

#include <Vulk/Device.h>

namespace {
std::vector<char> readFile(const std::string& filename) {
  std::ifstream inputFile(filename, std::ios::binary | std::ios::ate);

  MI_VERIFY_MSG(inputFile, "Failed to open file '%s'", filename.c_str());

  std::streamsize fileSize = inputFile.tellg();
  inputFile.seekg(0, std::ios::beg);

  std::vector<char> charVector;
  charVector.reserve(fileSize);

  charVector.assign(std::istreambuf_iterator<char>(inputFile), {});

  inputFile.close();
  return charVector;
}

} // namespace

NAMESPACE_VULKAN_BEGIN

ShaderModule::ShaderModule(const Device& device, const std::vector<char>& codes, const char* entry) {
  create(device, codes, entry);
}

ShaderModule::ShaderModule(const Device& device, const char* shaderFile, const char* entry) {
  create(device, shaderFile, entry);
}

ShaderModule::~ShaderModule() {
  if (isCreated()) {
    destroy();
  }
}

void ShaderModule::create(const Device& device, const std::vector<char>& codes, const char* entry) {
  MI_VERIFY(!isCreated());
  _device = &device;
  _entry = entry;

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = codes.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(codes.data());

  MI_VERIFY_VKCMD(vkCreateShaderModule(device, &createInfo, nullptr, &_shader));
}

void ShaderModule::create(const Device& device, const char* shaderFile, const char* entry) {
  create(device, readFile(shaderFile), entry);
}

void ShaderModule::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyShaderModule(*_device, _shader, nullptr);

  _shader = VK_NULL_HANDLE;
  _device = nullptr;
}

ShaderModule::ShaderModule(ShaderModule&& rhs) noexcept {
  moveFrom(rhs);
}

ShaderModule& ShaderModule::operator=(ShaderModule&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void ShaderModule::moveFrom(ShaderModule& rhs) {
  MI_VERIFY(!isCreated());
  _shader = rhs._shader;
  _entry = rhs._entry;
  _descriptorSetLayoutBindings = std::move(rhs._descriptorSetLayoutBindings);
  _device = rhs._device;

  rhs._shader = VK_NULL_HANDLE;
  rhs._entry = nullptr;
  rhs._descriptorSetLayoutBindings.clear();
  rhs._device = nullptr;
}

void ShaderModule::addDescriptorSetLayoutBinding(uint32_t binding,
                                                 VkDescriptorType descriptorType,
                                                 VkShaderStageFlags stageFlags) {
  _descriptorSetLayoutBindings.push_back({binding, descriptorType, 1, stageFlags, nullptr});
}

NAMESPACE_VULKAN_END
