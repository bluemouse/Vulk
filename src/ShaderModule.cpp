#include <Vulk/ShaderModule.h>

#include <fstream>
#include <vector>
#include <iostream>

#include <Vulk/Device.h>
#include <Vulk/TypeTraits.h>

#define PRINT_SPIRV_REFLECT_INFO

#include <spirv_reflect.h>

namespace {
[[nodiscard]] std::string typenameof(const SpvReflectDescriptorBinding& binding) {
  if (binding.type_description->type_name) {
    return binding.type_description->type_name;
  } else {
    if (binding.type_description->op == SpvOpTypeSampledImage) {
      switch (binding.image.dim) {
        case SpvDim1D:
          return "sampler1D";
        case SpvDim2D:
          return "sampler2D";
        case SpvDim3D:
          return "sampler3D";
        case SpvDimCube:
          return "samplerCube";
        case SpvDimRect:
          return "sampler2DRect";
        case SpvDimBuffer:
          return "samplerBuffer";
        default:
          return {};
      }
    }
  }
  return {};
}
} // namespace

#if defined(PRINT_SPIRV_REFLECT_INFO)
// Helpers for SpirvReflect
#  include <common/output_stream.h>
#  include <common/output_stream.cpp>

std::string ToStringSpvReflectShaderStage(SpvReflectShaderStageFlagBits stage) {
  switch (stage) {
    default:
      return "Unknown";
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
      return "Vertex";
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
      return "Tessellation Control";
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
      return "Tessellation Evaluation";
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
      return "Geometry";
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
      return "Fragment";
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
      return "Compute";
  }
}

std::string ToStringQualifier(SpvReflectDecorationFlags qualifier) {
  if (qualifier & SPV_REFLECT_DECORATION_FLAT) {
    return "flat";
  } else if (qualifier & SPV_REFLECT_DECORATION_NOPERSPECTIVE) {
    return "noperspective";
  } else if (qualifier & SPV_REFLECT_DECORATION_PATCH) {
    return "patch";
  } else if (qualifier & SPV_REFLECT_DECORATION_PER_VERTEX) {
    return "pervertex";
  } else if (qualifier & SPV_REFLECT_DECORATION_PER_TASK) {
    return "pertask";
  }
  return "";
}

namespace {
[[maybe_unused]] void PrintModuleInfo(std::ostream& os, const SpvReflectShaderModule& module) {
  os << "  entry point     : " << module.entry_point_name << "\n";
  os << "  source lang     : " << spvReflectSourceLanguage(module.source_language) << "\n";
  os << "  source lang ver : " << module.source_language_version << "\n";
  os << "  stage           : " << ToStringSpvReflectShaderStage(module.shader_stage) << "\n";
}

[[maybe_unused]] void PrintDescriptorBinding(std::ostream& os,
                                             const SpvReflectDescriptorBinding& binding) {
  os << "    name : " << binding.name << "\n";
  os << "      type name : " << typenameof(binding) << "\n";
  os << "      binding   : " << binding.binding << "\n";
  os << "      set       : " << binding.set << "\n";
  os << "      type      : " << ToStringDescriptorType(binding.descriptor_type) << "\n";

  if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
      binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
    std::vector<TextLine> text_lines;
    ParseBlockVariableToTextLines("    ", false, binding.block, &text_lines);
    if (!text_lines.empty()) {
      os << "\n";
      StreamWriteTextLines(os, "  ", false, text_lines);
      os << "\n";
    }
  }

  // array
  if (binding.array.dims_count > 0) {
    os << "      array    : ";
    for (uint32_t idx = 0; idx < binding.array.dims_count; ++idx) {
      os << "[" << binding.array.dims[idx] << "]";
    }
    os << "\n";
  }

  // counter
  if (binding.uav_counter_binding != nullptr) {
    os << "      counter  : "
       << "( set=" << binding.uav_counter_binding->set
       << ", binding=" << binding.uav_counter_binding->binding
       << ", name=" << binding.uav_counter_binding->name << " );\n";
  }
}

[[maybe_unused]] void PrintDescriptorSet(std::ostream& os, const SpvReflectDescriptorSet& desc) {
  os << "  set           : " << desc.set << "\n";
  os << "  binding count : " << desc.binding_count << "\n";
  for (uint32_t idx = 0; idx < desc.binding_count; ++idx) {
    PrintDescriptorBinding(os, *desc.bindings[idx]);
  }
  os << "\n";
}

[[maybe_unused]] void PrintInterfaceVariable(std::ostream& os,
                                             SpvSourceLanguage src_lang,
                                             const SpvReflectInterfaceVariable& variable) {
  if (variable.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
    // Ignore the build in variables such as gl_PerVertex
    return;
  }

  os << "  name : " << variable.name << "\n";
  if (variable.type_description->type_name) {
    os << "    type name : " << variable.type_description->type_name << "\n";
  }
  os << "    location  : " << variable.location << "\n";

  if (variable.semantic) {
    os << "    semantic  : " << variable.semantic << "\n";
  }
  os << "    type      : " << ToStringType(src_lang, *variable.type_description) << "\n";
  os << "    format    : " << ToStringFormat(variable.format) << "\n";
  os << "    qualifier : " << ToStringQualifier(variable.decoration_flags) << "\n";
}
} // namespace

#endif // PRINT_SPIRV_REFLECT_INFO

namespace {
std::vector<char> readFile(const std::string& filename) {
  std::ifstream inputFile(filename, std::ios::binary | std::ios::ate);

  MI_VERIFY_MSG(inputFile, "Failed to open file '%s'", filename.c_str());

  std::streamsize fileSize = inputFile.tellg();
  std::streamsize padding4 = (4 - fileSize % 4);
  if (padding4 > 0) {
    fileSize += padding4;
  }

  inputFile.seekg(0, std::ios::beg);

  std::vector<char> charVector;
  charVector.reserve(fileSize);

  // Make sure the codes data are aligned to 4 bytes. The padding should be filled with 0.
  charVector.assign(std::istreambuf_iterator<char>(inputFile), {});
  for (std::streamsize i = 0; i < padding4; ++i) {
    charVector.push_back(0);
  }
  charVector.resize(fileSize - padding4);

  inputFile.close();
  return charVector;
}
} // namespace

NAMESPACE_Vulk_BEGIN

ShaderModule::ShaderModule(const Device& device, const std::vector<char>& codes, bool reflection) {
  create(device, codes, reflection);
}

ShaderModule::ShaderModule(const Device& device, const char* shaderFile, bool reflection) {
  create(device, shaderFile, reflection);
}

ShaderModule::~ShaderModule() {
  if (isCreated()) {
    destroy();
  }
}

void ShaderModule::create(const Device& device, const std::vector<char>& codes, bool reflection) {
  MI_VERIFY(!isCreated());
  _device = &device;
  _entry = "main";

  if (reflection) {
    reflectShader(codes);
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = codes.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(codes.data());

  MI_VERIFY_VKCMD(vkCreateShaderModule(device, &createInfo, nullptr, &_shader));
}

void ShaderModule::create(const Device& device, const char* shaderFile, bool reflection) {
  create(device, readFile(shaderFile), reflection);
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

#define MI_VERIFY_SPVREFLECT_CMD(cmd)                                    \
  if (cmd != SPV_REFLECT_RESULT_SUCCESS) {                               \
    throw std::runtime_error("Error: " #cmd " failed" _MI_AT_THIS_LINE); \
  }

void ShaderModule::reflectShader(const std::vector<char>& codes) {
  SpvReflectShaderModule module = {};
  MI_VERIFY_SPVREFLECT_CMD(spvReflectCreateShaderModule(codes.size(), codes.data(), &module));
  _entry = module.entry_point_name;

#if defined(PRINT_SPIRV_REFLECT_INFO)
  std::cout << "Module:\n";
  PrintModuleInfo(std::cout, module);
#endif

  reflectDescriptorSets(module);
  reflectVertexInputs(module);

#if defined(PRINT_SPIRV_REFLECT_INFO)
  // OutputVariables
  uint32_t count = 0;
  MI_VERIFY_SPVREFLECT_CMD(spvReflectEnumerateOutputVariables(&module, &count, nullptr));
  std::vector<SpvReflectInterfaceVariable*> outVars(count);
  MI_VERIFY_SPVREFLECT_CMD(spvReflectEnumerateOutputVariables(&module, &count, outVars.data()));

  std::cout << "Output variables:\n";
  for (auto* var : outVars) {
    PrintInterfaceVariable(std::cout, module.source_language, *var);
  }
#endif

  spvReflectDestroyShaderModule(&module);
}

void ShaderModule::reflectDescriptorSets(const SpvReflectShaderModule& module) {
  struct DescriptorSetLayoutData {
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
  };

  uint32_t count = 0;
  MI_VERIFY_SPVREFLECT_CMD(spvReflectEnumerateDescriptorSets(&module, &count, nullptr));
  std::vector<SpvReflectDescriptorSet*> descriptorSets{count};
  MI_VERIFY_SPVREFLECT_CMD(
      spvReflectEnumerateDescriptorSets(&module, &count, descriptorSets.data()));

#if defined(PRINT_SPIRV_REFLECT_INFO)
  std::cout << "Descriptor sets:\n";
  for (auto& set : descriptorSets) {
    PrintDescriptorSet(std::cout, *set);
  }
#endif

  for (const auto* descriptorSet : descriptorSets) {
    DescriptorSetLayoutBindingBindingMeta layoutBindingMeta;
    VkDescriptorSetLayoutBinding layoutBinding{};
    for (uint32_t bindingIdx = 0; bindingIdx < descriptorSet->binding_count; ++bindingIdx) {
      const auto& descriptorBinding = *(descriptorSet->bindings[bindingIdx]);
      layoutBindingMeta.name = descriptorBinding.name;
      layoutBindingMeta.typeName = typenameof(descriptorBinding);
      layoutBinding.binding = descriptorBinding.binding;
      layoutBinding.descriptorType =
          static_cast<VkDescriptorType>(descriptorBinding.descriptor_type);
      layoutBinding.descriptorCount = 1;
      for (uint32_t dimIdx = 0; dimIdx < descriptorBinding.array.dims_count; ++dimIdx) {
        layoutBinding.descriptorCount *= descriptorBinding.array.dims[dimIdx];
      }
      layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);

      _descriptorSetLayoutBindingsMeta.push_back(layoutBindingMeta);
      _descriptorSetLayoutBindings.push_back(layoutBinding);
    }
  }
}

void ShaderModule::reflectVertexInputs(const SpvReflectShaderModule& module) {
  uint32_t count = 0;
  MI_VERIFY_SPVREFLECT_CMD(spvReflectEnumerateInputVariables(&module, &count, nullptr));
  std::vector<SpvReflectInterfaceVariable*> inVars(count);
  MI_VERIFY_SPVREFLECT_CMD(spvReflectEnumerateInputVariables(&module, &count, inVars.data()));

#if defined(PRINT_SPIRV_REFLECT_INFO)
  std::cout << "Input variables:\n";
  for (auto* var : inVars) {
    PrintInterfaceVariable(std::cout, module.source_language, *var);
  }
#endif

  if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
    // Simplifying assumptions:
    // - All vertex input attributes are sourced from a single vertex buffer,
    //   bound to VB slot 0.
    // - Each vertex's attribute are laid out in ascending order by location.
    // - The format of each attribute matches its usage in the shader;
    //   float4 -> VK_FORMAT_R32G32B32A32_FLOAT, etc. No attribute compression
    //   is applied.
    // - All attributes are provided per-vertex, not per-instance.
    VkVertexInputBindingDescription bindingDescription{0, 0, VK_VERTEX_INPUT_RATE_VERTEX};

    _vertexInputAttributes.reserve(inVars.size());
    for (const auto* var : inVars) {
      // ignore built-in variables
      if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
        continue;
      }
      VkVertexInputAttributeDescription desc{};
      desc.location = var->location;
      desc.binding = bindingDescription.binding;
      desc.format = static_cast<VkFormat>(var->format);
      desc.offset = 0; // final offset computed below after sorting.
      _vertexInputAttributes.push_back(desc);
    }

    // Sort attributes by location
    std::sort(std::begin(_vertexInputAttributes),
              std::end(_vertexInputAttributes),
              [](const VkVertexInputAttributeDescription& a,
                 const VkVertexInputAttributeDescription& b) { return a.location < b.location; });

    // Compute final offsets of each attribute, and total vertex stride.
    for (auto& attribute : _vertexInputAttributes) {
      attribute.offset = bindingDescription.stride;
      bindingDescription.stride += formatsizeof(attribute.format);
    }

    _vertexInputBindings.push_back(bindingDescription);
  }
}

NAMESPACE_Vulk_END
