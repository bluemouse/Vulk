#pragma once

#include "MainWindow.h"
#include "Camera.h"

#include <Vulk/Framebuffer.h>
#include <Vulk/Fence.h>

#include <Vulk/engine/Context.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Texture2D.h>
#include <Vulk/engine/TypeTraits.h>

class Testbed : public MainWindow {
 public:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& rhs) const {
      return pos == rhs.pos && color == rhs.color && texCoord == rhs.texCoord;
    }

    static VkVertexInputBindingDescription bindingDescription(uint32_t binding) {
      return {binding, Vertex::size(), VK_VERTEX_INPUT_RATE_VERTEX};
    }
    static std::vector<VkVertexInputAttributeDescription> attributesDescription(uint32_t binding) {
      // In shader.vert, we have:
      // layout(location = 0) in vec3 inPos;
      // layout(location = 1) in vec3 inColor;
      // layout(location = 2) in vec2 inTexCoord;
      return {{0, binding, formatof(Vertex::pos), offsetof(Vertex, pos)},
              {1, binding, formatof(Vertex::color), offsetof(Vertex, color)},
              {2, binding, formatof(Vertex::texCoord), offsetof(Vertex, texCoord)}};
    }
    static constexpr uint32_t size() { return sizeof(Vertex); }
  };

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

 private:
  Vulk::Context _context;

  uint32_t _vertexBufferBinding = 0U;

  struct Transformation {
    alignas(sizeof(glm::vec4)) glm::mat4 model;
    alignas(sizeof(glm::vec4)) glm::mat4 view;
    alignas(sizeof(glm::vec4)) glm::mat4 proj;

    static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
      return {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    }
  };
  Camera _camera;

  Vulk::Drawable<Vertex, uint32_t> _drawable;
  Vulk::Texture2D::shared_ptr _texture;

  struct Frame {
    Vulk::CommandBuffer commandBuffer;

    Vulk::Image2D::shared_ptr colorBuffer;
    Vulk::ImageView colorAttachment;
    Vulk::DepthImage::shared_ptr depthBuffer;
    Vulk::ImageView depthAttachment;
    Vulk::Framebuffer framebuffer;

    Vulk::UniformBuffer uniformBuffer;
    void* uniformBufferMapped;

    Vulk::DescriptorSet descriptorSet;

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
