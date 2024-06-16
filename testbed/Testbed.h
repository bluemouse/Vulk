#pragma once

#include "MainWindow.h"

#include <Vulk/Framebuffer.h>
#include <Vulk/Fence.h>

#include <Vulk/engine/Context.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Texture2D.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

class Testbed : public MainWindow {
 public:
  using Vertex = Vulk::Vertex<glm::vec3, glm::vec3, glm::vec2>;

 public:
  void init(int width, int height) override;
  void cleanup() override;
  void run() override;

  void drawFrame() override;

  void setModelFile(const std::string& modelFile);
  void setTextureFile(const std::string& textureFile);

  // Settings of the Testbed execution
  using ValidationLevel = Vulk::Context::ValidationLevel;
  static void setValidationLevel(ValidationLevel level);
  static void setPrintReflect(bool print);

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

  [[nodiscard]] static VkFormat chooseDepthFormat();

  void onKeyInput(int key, int action, int mods) override;
  void onMouseMove(double xpos, double ypos) override;
  void onMouseButton(int button, int action, int mods) override;
  void onScroll(double xoffset, double yoffset) override;
  void onFramebufferResize(int width, int height) override;

 private:
  void createContext();
  void createDrawable();
  void createFrames();

  void updateUniformBuffer();

  void nextFrame();

  void loadModel(const std::string& modelFile,
                 std::vector<Vertex>& vertices,
                 std::vector<uint32_t>& indices);
  void initCamera(const std::vector<Vertex>& vertices);

  virtual void renderFrame(Vulk::CommandBuffer& commandBuffer,
                           Vulk::Framebuffer& framebuffer,
                           const std::vector<Vulk::Semaphore*> waits,
                           const std::vector<Vulk::Semaphore*> signals,
                           Vulk::Fence& fence);
  virtual void presentFrame(Vulk::CommandBuffer& commandBuffer,
                            Vulk::Framebuffer& framebuffer,
                            const std::vector<Vulk::Semaphore*> waits);

 private:
  Vulk::Context _context;

  uint32_t _vertexBufferBinding = 0U;

  Vulk::Camera _camera;

  Vulk::Drawable<Vertex, uint32_t> _drawable;
  Vulk::Texture2D::shared_ptr _texture;

  struct Frame {
    Vulk::CommandBuffer::shared_ptr commandBuffer;

    Vulk::Image2D::shared_ptr colorBuffer;
    Vulk::ImageView::shared_ptr colorAttachment;
    Vulk::DepthImage::shared_ptr depthBuffer;
    Vulk::ImageView::shared_ptr depthAttachment;
    Vulk::Framebuffer::shared_ptr framebuffer;

    Vulk::UniformBuffer::shared_ptr uniformBuffer;
    void* uniformBufferMapped;

    Vulk::DescriptorSet::shared_ptr descriptorSet;

    Vulk::Semaphore::shared_ptr imageAvailableSemaphore;
    Vulk::Semaphore::shared_ptr renderFinishedSemaphore;
    Vulk::Fence::shared_ptr fence;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  uint32_t _currentFrameIdx  = 0;
  uint32_t _maxFrameInFlight = 2;

  // Settings of the Testbed execution
  static ValidationLevel _validationLevel;

  // UI control variables
  float _zoomFactor = 1.0F;

  // Input data
  std::string _modelFile{};
  std::string _textureFile{};
};
