#pragma once

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/Surface.h>
#include <Vulk/Swapchain.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/Buffer.h>
#include <Vulk/UniformBuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/Sampler.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Context {
 public:
  using ValidationLevel       = Instance::ValidationLevel;
  using ChooseDepthFormatFunc = std::function<VkFormat()>;

 public:
  using CreateWindowSurfaceFunc = std::function<VkSurfaceKHR(const Vulk::Instance& instance)>;
  using CreateVertShaderFunc    = std::function<Vulk::VertexShader(const Vulk::Device& device)>;
  using CreateFragShaderFunc    = std::function<Vulk::FragmentShader(const Vulk::Device& device)>;
  struct CreateInfo {
    int versionMajor = 1;
    int versionMinor = 0;
    std::vector<const char*> extensions;
    ValidationLevel validationLevel = ValidationLevel::kNone;

    PhysicalDevice::IsDeviceSuitableFunc isDeviceSuitable;

    CreateWindowSurfaceFunc createWindowSurface;

    Swapchain::ChooseSurfaceExtentFunc chooseSurfaceExtent;
    Swapchain::ChooseSurfaceFormatFunc chooseSurfaceFormat;
    Swapchain::ChoosePresentModeFunc choosePresentMode;

    ChooseDepthFormatFunc chooseDepthFormat;

    uint32_t maxDescriptorSets = 2;

    CreateVertShaderFunc createVertShader;
    CreateFragShaderFunc createFragShader;
  };

 public:
  Context() = default;

  virtual ~Context() = default;

  Context(const Context& rhs)            = delete;
  Context& operator=(const Context& rhs) = delete;

  virtual void create(const CreateInfo& createInfo);
  virtual void destroy();

  void waitIdle() const;

  [[nodiscard]] bool isComplete() const;

  [[nodiscard]] Instance& instance() { return _instance; }
  [[nodiscard]] Surface& surface() { return _surface; }
  [[nodiscard]] Device& device() { return _device; }
  [[nodiscard]] Swapchain& swapchain() { return _swapchain; }
  [[nodiscard]] RenderPass& renderPass() { return _renderPass; }
  [[nodiscard]] Pipeline& pipeline() { return _pipeline; }
  [[nodiscard]] DescriptorPool& descriptorPool() { return _descriptorPool; }
  [[nodiscard]] CommandPool& commandPool() { return _commandPool; }

  [[nodiscard]] const Instance& instance() const { return _instance; }
  [[nodiscard]] const Surface& surface() const { return _surface; }
  [[nodiscard]] const Device& device() const { return _device; }
  [[nodiscard]] const Swapchain& swapchain() const { return _swapchain; }
  [[nodiscard]] const RenderPass& renderPass() const { return _renderPass; }
  [[nodiscard]] const Pipeline& pipeline() const { return _pipeline; }
  [[nodiscard]] const DescriptorPool& descriptorPool() const { return _descriptorPool; }
  [[nodiscard]] const CommandPool& commandPool() const { return _commandPool; }

 protected:
  virtual void createInstance(int versionMajor,
                              int versionMinor,
                              const std::vector<const char*>& extensions,
                              ValidationLevel validation = ValidationLevel::kNone);
  virtual void createSurface(const CreateWindowSurfaceFunc& createWindowSurface);
  virtual void pickPhysicalDevice(const PhysicalDevice::IsDeviceSuitableFunc& isDeviceSuitable);
  virtual void createLogicalDevice();
  virtual void createRenderPass(const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                                const ChooseDepthFormatFunc& chooseDepthFormat = {});
  virtual void createSwapchain(const Swapchain::ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                               const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                               const Swapchain::ChoosePresentModeFunc& choosePresentMode);

  virtual void createPipeline(const CreateVertShaderFunc& createVertShader,
                              const CreateFragShaderFunc& createFragShader);

  virtual void createCommandPool();
  virtual void createDescriptorPool(uint32_t maxSets);

 protected:
  Vulk::Instance _instance;

  Vulk::Surface _surface;
  Vulk::Device _device;
  Vulk::Swapchain _swapchain;

  Vulk::RenderPass _renderPass;
  Vulk::Pipeline _pipeline;

  Vulk::DescriptorPool _descriptorPool;
  Vulk::CommandPool _commandPool;
};

NAMESPACE_END(Vulk)