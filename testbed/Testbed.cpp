#include "Testbed.h"

#include <Vulk/Toolbox.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/DepthImage.h>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS, GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>

#include <set>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>
#include <cstring>

namespace std {
template <>
struct hash<Testbed::Vertex> {
  size_t operator()(const Testbed::Vertex& vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std

Testbed::ValidationLevel Testbed::_validationLevel = ValidationLevel::kError;

void Testbed::setValidationLevel(ValidationLevel level) {
  _validationLevel = level;
}
void Testbed::setPrintReflect(bool print) {
  print ? Vulk::ShaderModule::enablePrintReflection()
        : Vulk::ShaderModule::disablePrintReflection();
}

namespace {
#if defined(__linux__)
std::filesystem::path executablePath() {
  return std::filesystem::canonical("/proc/self/exe").parent_path();
}
#else
std::filesystem::path getExecutablePath() {
  return std::filesystem::path{};
}
#endif
} // namespace

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createContext();
  createRenderable();
  createFrames();
}

void Testbed::cleanup() {
  _frames.clear();

  _texture.destroy();

  _drawable.destroy();
  _context.destroy();

  MainWindow::cleanup();
}

void Testbed::run() {
  MainWindow::run();
}

void Testbed::mainLoop() {
  MainWindow::mainLoop();
  _context.waitIdle();
}

void Testbed::drawFrame() {
  nextFrame();

  _currentFrame->fence.wait();

  if (_context.swapchain().acquireNextImage(_currentFrame->imageAvailableSemaphore) ==
      VK_ERROR_OUT_OF_DATE_KHR) {
    resizeSwapchain();
    return;
  }
  // Need to reset the fence after the potential swapchain resize. Otherwise, there will be no
  // fence signal command submitted and we could have a dead lock.
  _currentFrame->fence.reset();

  updateUniformBuffer();

  _currentFrame->commandBuffer.reset();
  _currentFrame->commandBuffer.executeCommands(
      [this](const Vulk::CommandBuffer& commandBuffer) {
        const auto& framebuffer = _context.swapchain().activeFramebuffer();

        commandBuffer.beginRenderPass(_context.renderPass(), framebuffer);

        commandBuffer.bindPipeline(_context.pipeline());

        auto extent = framebuffer.extent();
        commandBuffer.setViewport({0.0F, 0.0F}, {extent.width, extent.height});

        commandBuffer.bindVertexBuffer(_drawable.vertexBuffer(), _vertexBufferBinding);
        commandBuffer.bindIndexBuffer(_drawable.indexBuffer());
        commandBuffer.bindDescriptorSet(_context.pipeline(), _currentFrame->descriptorSet);

        commandBuffer.drawIndexed(_drawable.numIndices());

        commandBuffer.endRenderpass();
      },
      {&_currentFrame->imageAvailableSemaphore},
      {&_currentFrame->renderFinishedSemaphore},
      _currentFrame->fence);

  auto result = _context.swapchain().present(_currentFrame->renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized()) {
    resizeSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Error: failed to present swap chain image!");
  }
}

void Testbed::createContext() {
  auto [extensionCount, extensions] = getRequiredInstanceExtensions();

  Vulk::Context::CreateInfo createInfo;
  createInfo.extensions      = {extensions, extensions + extensionCount};
  createInfo.validationLevel = _validationLevel;

  createInfo.isDeviceSuitable = [this](VkPhysicalDevice device) {
    return isPhysicalDeviceSuitable(device, _context.surface());
  };
  createInfo.createWindowSurface = [this](const Vulk::Instance& instance) {
    return createWindowSurface(instance);
  };
  createInfo.chooseSurfaceExtent = [this](const VkSurfaceCapabilitiesKHR& caps) {
    return chooseSwapchainSurfaceExtent(caps, width(), height());
  };
  createInfo.chooseSurfaceFormat = &Testbed::chooseSwapchainSurfaceFormat;
  createInfo.choosePresentMode   = &Testbed::chooseSwapchainPresentMode;

  createInfo.chooseDepthFormat = &Testbed::chooseDepthFormat;

  createInfo.maxDescriptorSets = _maxFrameInFlight;
  createInfo.createVertShader  = &Testbed::createVertexShader;
  createInfo.createFragShader  = &Testbed::createFragmentShader;

  _context.create(createInfo);

  _vertexBufferBinding = _context.pipeline().findBinding<Vertex>();
  MI_VERIFY(_vertexBufferBinding != std::numeric_limits<uint32_t>::max());
}

void Testbed::createRenderable() {
#define DRAW_MODEL
#ifdef DRAW_MODEL
  const std::string MODEL_FILE   = "models/viking_room.obj";
  const std::string TEXTURE_FILE = "textures/viking_room.png";

  auto modelFile   = executablePath() / MODEL_FILE;
  auto textureFile = executablePath() / TEXTURE_FILE;

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFile.string().c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0F, 1.0F, 1.0F};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }

  _drawable.create(
      _context.device(), Vulk::CommandBuffer{_context.commandPool()}, vertices, indices);

  _texture = Vulk::Toolbox(_context).createTexture(textureFile.string().c_str());
#else
  const std::vector<Vertex> vertices = {{{-1.0F, -1.0F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
                                        {{-1.0F, 1.0F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
                                        {{1.0F, 1.0F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
                                        {{1.0F, -1.0F, 0.5F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}},

                                        {{-0.5F, -0.5F, 0.25F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
                                        {{-0.5F, 0.5F, 0.25F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
                                        {{0.5F, 0.5F, 0.75F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
                                        {{0.5F, -0.5F, 0.75F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}}};

  const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

  _drawable.create(
      _context.device(), Vulk::CommandBuffer{_context.commandPool()}, vertices, indices);

  auto textureFile = executablePath() / "textures/texture.jpg";
  _texture         = Vulk::Toolbox(_context).createTexture(textureFile.string().c_str());
#endif // DRAW_MODEL
}

void Testbed::createFrames() {
  _frames.resize(_maxFrameInFlight);

  VkDescriptorImageInfo textureImageInfo{};
  textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureImageInfo.imageView   = _texture;
  textureImageInfo.sampler     = _texture;

  VkDescriptorBufferInfo transformationBufferInfo{};
  transformationBufferInfo.offset = 0;
  transformationBufferInfo.range  = VK_WHOLE_SIZE;

  for (auto& frame : _frames) {
    frame.commandBuffer.allocate(_context.commandPool());
    frame.imageAvailableSemaphore.create(_context.device());
    frame.renderFinishedSemaphore.create(_context.device());
    frame.fence.create(_context.device(), true);

    frame.uniformBuffer.create(_context.device(), sizeof(Transformation));
    frame.uniformBufferMapped = frame.uniformBuffer.map();

    transformationBufferInfo.buffer = frame.uniformBuffer;

    // The order of bindings must match the order of bindings in shaders. The name and the type need
    // to match them in the shader as well.
    std::vector<Vulk::DescriptorSet::Binding> bindings = {
        {"xform", "Transformation", &transformationBufferInfo},
        {"texSampler", "sampler2D", &textureImageInfo}};
    frame.descriptorSet.allocate(
        _context.descriptorPool(), _context.pipeline().descriptorSetLayout(), bindings);
  }

  nextFrame();
}

Vulk::VertexShader Testbed::createVertexShader(const Vulk::Device& device) {
  auto shaderFile = executablePath() / "shaders/vert.spv";
  return {device, shaderFile.string().c_str()};
}

Vulk::FragmentShader Testbed::createFragmentShader(const Vulk::Device& device) {
  auto shaderFile = executablePath() / "shaders/frag.spv";
  return {device, shaderFile.string().c_str()};
}

void Testbed::resizeSwapchain() {
  if (isMinimized()) {
    return;
  }
  _context.waitIdle();
  _context.swapchain().resize(width(), height());

  setFramebufferResized(false);

  // To force a drawFrame()
  MainWindow::postEmptyEvent();
}

void Testbed::updateUniformBuffer() {
  using glm::vec3;
  using glm::vec4;
  using glm::mat4;

  auto [textureW, textureH] = _texture.extent();
  float textureAspect       = static_cast<float>(textureW) / static_cast<float>(textureH);

  Transformation xform{};

  // Make the aspect ratio of the rendered texture match the physical texture.
  xform.model = mat4{1.0F};
  if (textureAspect > 1.0F) {
    xform.model[1][1] = 1.0F / textureAspect;
  } else {
    xform.model[0][0] = 1.0F / textureAspect;
  }

  vec3 cameraPos{0.0F, 0.0F, -1.0F};
  vec3 cameraLookAt{0.0F, 0.0F, 0.0F};
  vec3 cameraUp{0.0F, -1.0F, 0.0F};

  xform.view = glm::lookAt(cameraPos, cameraLookAt, cameraUp);

  auto [surfaceW, surfaceH] = _context.swapchain().surfaceExtent();
  float surfaceAspect       = static_cast<float>(surfaceW) / static_cast<float>(surfaceH);

  vec4 roi = surfaceAspect > 1.0F ? vec4{-surfaceAspect, surfaceAspect, 1.0F, -1.0F}
                                  : vec4{-1.0F, 1.0F, 1.0F / surfaceAspect, -1.0F / surfaceAspect};

  auto dist  = glm::distance(cameraPos, cameraLookAt);
  auto zNear = dist;
  auto zFar  = zNear + dist * 10.0F;
// #define USE_PERSPECTIVE_PROJECTION
#if defined(USE_PERSPECTIVE_PROJECTION)
  float fovy = glm::angle(cameraPos - cameraLookAt, cameraUp * (roi[2] - roi[3]) / 2.0F);
  xform.proj = glm::perspective(fovy, surfaceAspect, zNear, zFar);
  xform.proj[1][1] *= -1;
#else
  xform.proj       = glm::ortho(roi[0], roi[1], roi[2], roi[3], zNear, zFar);
#endif

  memcpy(_currentFrame->uniformBufferMapped, &xform, sizeof(xform));
}

bool Testbed::isPhysicalDeviceSuitable(VkPhysicalDevice device, const Vulk::Surface& surface) {
  auto queueFamilies           = Vulk::PhysicalDevice::findQueueFamilies(device, surface);
  bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;

  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions{extensionCount};
  vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, availableExtensions.data());
  std::set<std::string> requiredExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  for (const auto& ext : availableExtensions) {
    requiredExtensions.erase(ext.extensionName);
  }
  bool extensionsSupported = requiredExtensions.empty();

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return isQueueFamiliesComplete && extensionsSupported && surface.isAdequate(device) &&
         (supportedFeatures.samplerAnisotropy != 0U);
}

VkExtent2D Testbed::chooseSwapchainSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                                 uint32_t windowWidth,
                                                 uint32_t windowHeight) {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  } else {
    auto width  = std::clamp(windowWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
    auto height = std::clamp(windowHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    return {width, height};
  }
}

VkSurfaceFormatKHR Testbed::chooseSwapchainSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR Testbed::chooseSwapchainPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkFormat Testbed::chooseDepthFormat() {
  constexpr uint32_t depthBits   = 24U;
  constexpr uint32_t stencilBits = 8U;
  return Vulk::DepthImage::findFormat(depthBits, stencilBits);
}

void Testbed::nextFrame() {
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFrameInFlight;
  _currentFrame    = &_frames[_currentFrameIdx];
}