#pragma once

#include "MainWindow.h"

#include <Vulk/Context.h>
#include <Vulk/Drawable.h>

class Testbed : public MainWindow {
 public:
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
  };

 public:
  void init(int width, int height) override;
  void cleanup() override;
  void run() override;

  void drawFrame() override;

 protected:
  void mainLoop() override;

  void resizeSwapchain();

  [[nodiscard]] Vulkan::Context& context() { return _context; }
  [[nodiscard]] const Vulkan::Context& context() const { return _context; }

  [[nodiscard]] VkSurfaceKHR createWindowSurface(VkInstance instance) {
    return MainWindow::createWindowSurface(instance);
  }

  [[nodiscard]] static bool isPhysicalDeviceSuitable(VkPhysicalDevice device,
                                                     const Vulkan::Surface& surface);
  [[nodiscard]] static VkExtent2D chooseSwapchainSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                                               uint32_t windowWidth,
                                                               uint32_t windowHeight);

  [[nodiscard]] static Vulkan::VertexShader createVertexShader(const Vulkan::Device& device);
  [[nodiscard]] static Vulkan::FragmentShader createFragmentShader(const Vulkan::Device& device);

  [[nodiscard]] static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  [[nodiscard]] static VkPresentModeKHR chooseSwapchainPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);

  void nextFrame();

 private:
  void createContext();

  void createRenderable();

  void createTextureImage();
  void createFrames();

  void updateUniformBuffer();

 private:
  Vulkan::Context _context;

  using IndexedDrawable = Vulkan::Drawable<Vertex, uint16_t>;
  IndexedDrawable _drawable;

  Vulkan::Image _textureImage;
  Vulkan::ImageView _textureImageView;
  Vulkan::Sampler _textureSampler;

  struct Frame {
    Vulkan::CommandBuffer commandBuffer;

    Vulkan::Semaphore imageAvailableSemaphore;
    Vulkan::Semaphore renderFinishedSemaphore;
    Vulkan::Fence fence;

    Vulkan::UniformBuffer uniformBuffer;
    void* uniformBufferMapped;

    Vulkan::DescriptorSet descriptorSet;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  uint32_t _currentFrameIdx = 0;
  uint32_t _maxFrameInFlight = 2;
};
