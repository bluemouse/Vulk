#pragma once

#include "MainWindow.h"

#include <Vulk/Context.h>
#include <Vulk/Drawable.h>
#include <Vulk/Texture2D.h>
#include <Vulk/TypeTraits.h>

class Testbed : public MainWindow {
 public:
  void init(int width, int height) override;
  void cleanup() override;
  void run() override;

  void drawFrame() override;

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

 private:
  void createContext();

  void createRenderable();

  void createFrames();

  void updateUniformBuffer();

  void nextFrame();

 private:
  Vulk::Context _context;

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

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
  uint32_t _vertexBufferBinding = 0U;

  struct Transformation {
    alignas(sizeof(glm::vec4)) glm::mat4 model;
    alignas(sizeof(glm::vec4)) glm::mat4 view;
    alignas(sizeof(glm::vec4)) glm::mat4 proj;

    static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
      return {binding,
              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              1,
              VK_SHADER_STAGE_VERTEX_BIT,
              nullptr};
    }
  };

  Vulk::Drawable<Vertex, uint16_t> _drawable;
  Vulk::Texture2D _texture;

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

  // Settings of the Testbed execution
  static ValidationLevel _validationLevel;
};
