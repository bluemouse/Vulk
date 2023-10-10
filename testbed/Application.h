#pragma once

#include "MainWindow.h"

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

#include <vector>

class Application : public MainWindow {
 public:
  void init(int width, int height) override;
  void cleanup() override;
  void run() override;

 protected:
  void mainLoop() override;

  virtual void initVulkan();
  virtual void cleanupVulkan();

  void createInstance();
  void createLogicalDevice();
  void createRenderPass();
  void createSwapchain();

  void createFrames();

  virtual void createPipeline();

  virtual Vulkan::VertexShader createVertexShader(const Vulkan::Device& device) = 0;
  virtual Vulkan::FragmentShader createFragmentShader(const Vulkan::Device& device) = 0;

  void resizeSwapChain();

  virtual VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) = 0;
  virtual VkPresentModeKHR chooseSwapchainPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) = 0;

  virtual bool isPhysicalDeviceSuitable(VkPhysicalDevice device, const Vulkan::Surface& surface);

  virtual void nextFrame();

 private:
  VkExtent2D surfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                           uint32_t windowWidth,
                           uint32_t windowHeight) const;

 protected:
  Vulkan::Instance _instance;

  Vulkan::Surface _surface;
  Vulkan::Device _device;
  Vulkan::Swapchain _swapchain;

  Vulkan::RenderPass _renderPass;
  Vulkan::Pipeline _pipeline;

  Vulkan::CommandPool _commandPool;

  struct Frame {
    Vulkan::CommandBuffer commandBuffer;

    Vulkan::Semaphore imageAvailableSemaphore;
    Vulkan::Semaphore renderFinishedSemaphore;
    Vulkan::Fence fence;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  uint32_t _currentFrameIdx = 0;
  uint32_t _maxFrameInFlight = 2;
};
