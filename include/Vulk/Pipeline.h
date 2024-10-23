#pragma once

#include <volk/volk.h>

#include <vector>
#include <limits>
#include <memory>

#include <Vulk/internal/base.h>

#include <Vulk/DescriptorSetLayout.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class RenderPass;
class VertexShader;
class FragmentShader;
class ComputeShader;

class Pipeline : public Sharable<Pipeline>, private NotCopyable {
 public:
  struct Configuration {
    Configuration();

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    struct Blend {
      bool enabled = false;

      VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      VkBlendOp colorBlendOp            = VK_BLEND_OP_ADD;
      VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      VkBlendOp alphaBlendOp            = VK_BLEND_OP_ADD;
    } blend;

    struct DepthTest {
      bool enabled = true;
      bool writeEnabled   = true;
      VkCompareOp compareOp = VK_COMPARE_OP_LESS;
    } depthTest;

    struct StencilTest {
      bool enabled = false;
      VkStencilOpState front{};
      VkStencilOpState back{};
      //TODO Add more stencil test parameters to directly config StencilOpState front and back
    } stencilTest;
  };

 public:
  Pipeline(const Device& device,
           const RenderPass& renderPass,
           const VertexShader& vertShader,
           const FragmentShader& fragShader,
           const Configuration& config = {});
  ~Pipeline() override;

  void create(const Device& device,
              const RenderPass& renderPass,
              const VertexShader& vertShader,
              const FragmentShader& fragShader,
              const Configuration& config = {});
  void create(const Device& device, const ComputeShader& compShader);
  void destroy();

  operator VkPipeline() const { return _pipeline; }
  [[nodiscard]] VkPipelineLayout layout() const { return _layout; }

  [[nodiscard]] bool isCreated() const { return _pipeline != VK_NULL_HANDLE; }

  [[nodiscard]] const DescriptorSetLayout::shared_ptr& descriptorSetLayout() const {
    return _descriptorSetLayout;
  }

  template <typename VertexInput>
  [[nodiscard]] uint32_t findBinding() const {
    // We are going to use the first binding that match the size of Vertex type.
    // TODO More robust test is to match the individual attributes.
    for (const auto& binding : _vertexInputBindings) {
      if (binding.stride == sizeof(VertexInput)) {
        return binding.binding;
      }
    }
    return std::numeric_limits<uint32_t>::max();
  }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkPipeline _pipeline     = VK_NULL_HANDLE;
  VkPipelineLayout _layout = VK_NULL_HANDLE;

  DescriptorSetLayout::shared_ptr _descriptorSetLayout;

  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)