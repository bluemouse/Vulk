#include "Testbed.h"

#include <Vulk/Toolbox.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/DepthImage.h>

#include <GLFW/glfw3.h>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS, GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>

#include <tiny_obj_loader.h>

#include <set>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <cmath>

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

struct Checkerboard : public std::vector<uint8_t> {
  Checkerboard(glm::uvec2 numBlocks,
               glm::uvec2 blockSize,
               glm::uvec4 black = {0, 0, 0, 255},
               glm::uvec4 white = {255, 255, 255, 255})
      : extent{numBlocks * blockSize} {
    resize(extent.x * extent.y * kNumColorChannels);

    for (unsigned int x = 0; x < extent.x; ++x) {
      for (unsigned int y = 0; y < extent.y; ++y) {
        auto p     = at(x, y);
        auto color = (((x & blockSize.x) == 0) ^ ((y & blockSize.y) == 0)) ? white : black;

        p[0] = color[0];
        p[1] = color[1];
        p[2] = color[2];
        p[3] = color[3];
      }
    }
  }

  glm::uvec4 operator()(uint32_t x, uint32_t y) {
    uint8_t* pixel = at(x, y);
    return {pixel[0], pixel[1], pixel[2], pixel[3]};
  }

  glm::uvec2 extent;

 private:
  static constexpr unsigned int kNumColorChannels = 4;
  uint8_t* at(uint32_t x, uint32_t y) { return data() + (x + y * extent.x) * kNumColorChannels; }
};

} // namespace

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createContext();
  createDrawable();
  createFrames();

  _zoomFactor = 1.0F;
}

void Testbed::cleanup() {
  for (auto& frame : _frames) {
    frame.colorBuffer.destroy();
    frame.colorAttachment.destroy();
    frame.depthBuffer.destroy();
    frame.depthAttachment.destroy();
    frame.framebuffer.destroy();
  }
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
        const auto& framebuffer = _currentFrame->framebuffer;

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

  _currentFrame->commandBuffer.waitIdle();

  auto& swapchainFramebuffer = _context.swapchain().activeFramebuffer();
  swapchainFramebuffer.image().blitFrom(_currentFrame->commandBuffer,
                                        _currentFrame->framebuffer.image());
  swapchainFramebuffer.image().transitToNewLayout(_currentFrame->commandBuffer,
                                                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

void Testbed::loadModel(const std::string& modelFile,
                        std::vector<Vertex>& vertices,
                        std::vector<uint32_t>& indices) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFile.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices;

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
}

void Testbed::initCamera(const std::vector<Vertex>& vertices) {
  auto bbox = Camera::BBox::null();
  for (const auto& vertex : vertices) {
    bbox += vertex.pos;
  }
  bbox.expandPlanarSide(1.0F);

  auto extent = _context.swapchain().surfaceExtent();
  _camera.init(glm::vec2{extent.width, extent.height}, bbox);
}

void Testbed::createDrawable() {
  if (_textureFile.empty()) {
    Checkerboard checkerboard{{4, 4}, {128, 128}, {60, 60, 60, 255}};
    _texture = Vulk::Toolbox(_context).createTexture2D(Vulk::Toolbox::TextureFormat::RGBA,
                                                       checkerboard.data(),
                                                       checkerboard.extent.x,
                                                       checkerboard.extent.y);
  } else {
    _texture = Vulk::Toolbox(_context).createTexture2D(_textureFile.c_str());
  }

  if (_modelFile.empty()) {
    float left{-1.0F}, right{1.0F}, bottom{-1.0F}, top{1.0F};

    if (_texture.isValid()) {
      // to make sure the texture and the quad has the same aspect ratio
      auto [textureW, textureH] = _texture.extent();
      float textureAspect       = static_cast<float>(textureW) / static_cast<float>(textureH);

      if (textureAspect > 1.0F) {
        left  = -textureAspect;
        right = textureAspect;
      } else {
        bottom = -1.0F / textureAspect;
        top    = 1.0F / textureAspect;
      }
    }
    const std::vector<Vertex> vertices = {
        {{left, bottom, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{left, top, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
        {{right, top, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{right, bottom, 0.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}}};

    const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

    _drawable.create(_context.device(), {_context.commandPool()}, vertices, indices);

    initCamera(vertices);
  } else {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    loadModel(_modelFile, vertices, indices);

    _drawable.create(_context.device(), {_context.commandPool()}, vertices, indices);

    initCamera(vertices);
  }
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

  const auto& device = _context.device();
  const auto& extent = _context.swapchain().surfaceExtent();
  for (auto& frame : _frames) {
    frame.commandBuffer.allocate(_context.commandPool());

    const auto usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    frame.colorBuffer.create(device, VK_FORMAT_B8G8R8A8_SRGB, extent, usage);
    frame.colorBuffer.allocate();
    frame.colorAttachment.create(device, frame.colorBuffer);
    frame.depthBuffer.create(device, extent, chooseDepthFormat());
    frame.depthBuffer.allocate();
    frame.depthAttachment.create(device, frame.depthBuffer);
    frame.framebuffer.create(
        device, _context.renderPass(), frame.colorAttachment, frame.depthAttachment);

    frame.imageAvailableSemaphore.create(device);
    frame.renderFinishedSemaphore.create(device);
    frame.fence.create(device, true);

    frame.uniformBuffer.create(device, sizeof(Transformation));
    frame.uniformBufferMapped = frame.uniformBuffer.map();

    transformationBufferInfo.buffer = frame.uniformBuffer;

    // The order of bindings must match the order of bindings in shaders. The name and the type need
    // to match them in the shader as well.
    std::vector<Vulk::DescriptorSet::Binding> bindings = {
        {"xform", "Transformation", &transformationBufferInfo},
        {"texSampler", "sampler2D", &textureImageInfo}};
    frame.descriptorSet.allocate(
        _context.descriptorPool(), _context.pipeline().descriptorSetLayout(), bindings);

    frame.colorBuffer.transitToNewLayout(frame.commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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

  // We need to get the updated size from the surface directly. It is not guaranteed that the extent
  // is the same as the {width(), height()}.
  auto extent = _context.swapchain().surfaceExtent();
  _camera.update(glm::vec2{extent.width, extent.height});

  setFramebufferResized(false);

  // To force a drawFrame()
  MainWindow::postEmptyEvent();
}

void Testbed::updateUniformBuffer() {
#define USE_ARC_CAMERA
#if defined(USE_ARC_CAMERA)
  _camera.update();

  Transformation xform{};

  xform.model = glm::mat4{1.0F};
  xform.view  = _camera.viewMatrix();
  xform.proj  = _camera.projectionMatrix();

  memcpy(_currentFrame->uniformBufferMapped, &xform, sizeof(xform));
#else
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
  std::cout << "view: " << glm::to_string(xform.view) << std::endl;

  auto [surfaceW, surfaceH] = _context.swapchain().surfaceExtent();
  float surfaceAspect       = static_cast<float>(surfaceW) / static_cast<float>(surfaceH);

  vec4 roi = surfaceAspect > 1.0F ? vec4{-surfaceAspect, surfaceAspect, 1.0F, -1.0F}
                                  : vec4{-1.0F, 1.0F, 1.0F / surfaceAspect, -1.0F / surfaceAspect};

  auto dist  = glm::distance(cameraPos, cameraLookAt);
  auto zNear = dist;
  auto zFar  = zNear + dist * 10.0F;
// #define USE_PERSPECTIVE_PROJECTION
#  if defined(USE_PERSPECTIVE_PROJECTION)
  float fovy = glm::angle(cameraPos - cameraLookAt, cameraUp * (roi[2] - roi[3]) / 2.0F);
  xform.proj = glm::perspective(fovy, surfaceAspect, zNear, zFar);
  xform.proj[1][1] *= -1;
#  else
  xform.proj = glm::ortho(roi[0], roi[1], roi[2], roi[3], zNear, zFar);
  std::cout << "projection: " << glm::to_string(xform.proj) << std::endl;
#  endif

  memcpy(_currentFrame->uniformBufferMapped, &xform, sizeof(xform));
#endif // USE_ARC_CAMERA
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

void Testbed::onKeyInput(int key, int action, int mods) {
  MainWindow::onKeyInput(key, action, mods);

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_EQUAL && mods == GLFW_MOD_CONTROL) {
      _camera.zoom(_zoomFactor = 1.0F);
    } else if (key == GLFW_KEY_UP && mods == GLFW_MOD_CONTROL) {
      _camera.orbitVertical(M_PI / 2.0F);
    } else if (key == GLFW_KEY_DOWN && mods == GLFW_MOD_CONTROL) {
      _camera.orbitVertical(-M_PI / 2.0F);
    } else if (key == GLFW_KEY_RIGHT && mods == GLFW_MOD_CONTROL) {
      _camera.orbitHorizontal(M_PI / 2.0F);
    } else if (key == GLFW_KEY_LEFT && mods == GLFW_MOD_CONTROL) {
      _camera.orbitHorizontal(-M_PI / 2.0F);
    }
  }
}

namespace {
glm::vec2 startingMousePos{};
std::queue<glm::vec2> mousePosHistory{};
} // namespace

void Testbed::onMouseMove(double xpos, double ypos) {
  MainWindow::onMouseMove(xpos, ypos);

  int button = getMouseButton();
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    _camera.move(mousePosHistory.front(), {xpos, ypos});
    mousePosHistory.pop();
    mousePosHistory.push({xpos, ypos});
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (mousePosHistory.size() >= 2) {
      _camera.rotate(mousePosHistory.front(), {xpos, ypos});
      mousePosHistory = {};
      mousePosHistory.push({xpos, ypos});
    } else {
      mousePosHistory.push({xpos, ypos});
    }
  }
}

void Testbed::onMouseButton(int button, int action, int mods) {
  MainWindow::onMouseButton(button, action, mods);

  if (action == GLFW_PRESS) {
    double xpos{0}, ypos{0};
    glfwGetCursorPos(window(), &xpos, &ypos);
    startingMousePos = {xpos, ypos};
    mousePosHistory  = {};
    mousePosHistory.push({xpos, ypos});
  }
}

void Testbed::onScroll(double xoffset, double yoffset) {
  MainWindow::onScroll(xoffset, yoffset);

  int mods = getKeyModifier();

  float delta = 0.05;
  // Get the current key modifier state.
  if (mods == GLFW_MOD_SHIFT) {
    delta *= 2; // Accelerates the zoom speed
  }

  if (yoffset > 0.0F) {
    _camera.zoom(_zoomFactor *= (1.0F + delta));
  } else {
    _camera.zoom(_zoomFactor *= (1.0F - delta));
  }
}

void Testbed::onFramebufferResize(int width, int height) {
  MainWindow::onFramebufferResize(width, height);
}

namespace {
std::string locateFile(const std::string& file) {
  if (std::filesystem::exists(file)) {
    return file;
  } else {
    auto path = executablePath() / file;
    if (std::filesystem::exists(path)) {
      return path;
    }
  }
  return {};
}
} // namespace

void Testbed::setModelFile(const std::string& modelFile) {
  _modelFile = locateFile(modelFile);
  if (_modelFile.empty()) {
    throw std::runtime_error("Error: model file [" + modelFile + "] does not exist!");
  }

  std::cout << "Model file: " << _modelFile << std::endl;
}
void Testbed::setTextureFile(const std::string& textureFile) {
  _textureFile = locateFile(textureFile);
  if (_textureFile.empty()) {
    throw std::runtime_error("Error: texture file [" + textureFile + "] does not exist!");
  }

  std::cout << "Texture file: " << _textureFile << std::endl;
}
