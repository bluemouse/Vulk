#pragma once

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/Surface.h>
#include <Vulk/Swapchain.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
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
    // The info in CreateInfo is used to create the Vulkan context and listed in the usage order.
    int versionMajor = 1;
    int versionMinor = 0;
    std::vector<const char*> instanceExtensions;
    ValidationLevel validationLevel = ValidationLevel::kNone;

    CreateWindowSurfaceFunc createWindowSurface;

    PhysicalDevice::QueueFamilies queueFamilies; // To specify the queue families to be created
    std::vector<const char*> deviceExtensions;   // To specify the required device extensions
    PhysicalDevice::HasDeviceFeaturesFunc hasPhysicalDeviceFeatures;

    Swapchain::ChooseSurfaceFormatFunc chooseSurfaceFormat;
    ChooseDepthFormatFunc chooseDepthFormat;
    Swapchain::ChooseSurfaceExtentFunc chooseSurfaceExtent;
    Swapchain::ChoosePresentModeFunc choosePresentMode;


    CreateVertShaderFunc createVertShader;
    CreateFragShaderFunc createFragShader;

    uint32_t maxDescriptorSets;
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

  [[nodiscard]] Instance& instance() { return *_instance; }
  [[nodiscard]] Surface& surface() { return *_surface; }
  [[nodiscard]] Device& device() { return *_device; }
  [[nodiscard]] Swapchain& swapchain() { return *_swapchain; }
  [[nodiscard]] RenderPass& renderPass() { return *_renderPass; }
  [[nodiscard]] Pipeline& pipeline() { return *_pipeline; }
  [[nodiscard]] DescriptorPool& descriptorPool() { return *_descriptorPool; }
  [[nodiscard]] CommandPool& commandPool() { return *_commandPool; }

  [[nodiscard]] const Instance& instance() const { return *_instance; }
  [[nodiscard]] const Surface& surface() const { return *_surface; }
  [[nodiscard]] const Device& device() const { return *_device; }
  [[nodiscard]] const Swapchain& swapchain() const { return *_swapchain; }
  [[nodiscard]] const RenderPass& renderPass() const { return *_renderPass; }
  [[nodiscard]] const Pipeline& pipeline() const { return *_pipeline; }
  [[nodiscard]] const DescriptorPool& descriptorPool() const { return *_descriptorPool; }
  [[nodiscard]] const CommandPool& commandPool() const { return *_commandPool; }

 protected:
  virtual void createInstance(int versionMajor,
                              int versionMinor,
                              const std::vector<const char*>& extensions,
                              ValidationLevel validation = ValidationLevel::kNone);
  virtual void createSurface(const CreateWindowSurfaceFunc& createWindowSurface);
  virtual void pickPhysicalDevice(const PhysicalDevice::QueueFamilies& queueFamilies,
                                  const std::vector<const char*>& deviceExtensions,
                                  const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures);
  virtual void createLogicalDevice(const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
                                   const std::vector<const char*>& deviceExtensions);
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
  Vulk::Instance::shared_ptr _instance;

  Vulk::Surface::shared_ptr _surface;
  Vulk::Device::shared_ptr _device;
  Vulk::Swapchain::shared_ptr _swapchain;

  Vulk::RenderPass::shared_ptr _renderPass;
  Vulk::Pipeline::shared_ptr _pipeline;

  Vulk::DescriptorPool::shared_ptr _descriptorPool;
  Vulk::CommandPool::shared_ptr _commandPool;
};

NAMESPACE_END(Vulk)
