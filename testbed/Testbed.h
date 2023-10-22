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

  [[nodiscard]] Vulk::Context& context() { return _context; }
  [[nodiscard]] const Vulk::Context& context() const { return _context; }

  // Callbacks to support creating Context
  [[nodiscard]] VkSurfaceKHR createWindowSurface(const Vulk::Instance& instance) {
    return MainWindow::createWindowSurface(instance);
  }

  [[nodiscard]] static bool isPhysicalDeviceSuitable(VkPhysicalDevice device,
                                                     const Vulk::Surface& surface);
  [[nodiscard]] static VkExtent2D chooseSwapchainSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                                               uint32_t windowWidth,
                                                               uint32_t windowHeight);

  [[nodiscard]] static Vulk::VertexShader createVertexShader(const Vulk::Device& device);
  [[nodiscard]] static Vulk::FragmentShader createFragmentShader(const Vulk::Device& device);

  [[nodiscard]] static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  [[nodiscard]] static VkPresentModeKHR chooseSwapchainPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);

 private:
  void createContext();

  void createRenderable();

    void createFrames();

  void updateUniformBuffer();

  void nextFrame();

 private:
  Vulk::Context _context;

  using IndexedDrawable = Vulk::Drawable<Vertex, uint16_t>;
  IndexedDrawable _drawable;

  Vulk::Image _textureImage;
  Vulk::ImageView _textureImageView;
  Vulk::Sampler _textureSampler;

  struct Frame {
    Vulk::CommandBuffer commandBuffer;

    Vulk::Semaphore imageAvailableSemaphore;
    Vulk::Semaphore renderFinishedSemaphore;
    Vulk::Fence fence;

    Vulk::UniformBuffer uniformBuffer;
    void* uniformBufferMapped;

    Vulk::DescriptorSet descriptorSet;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  uint32_t _currentFrameIdx = 0;
  uint32_t _maxFrameInFlight = 2;
};
