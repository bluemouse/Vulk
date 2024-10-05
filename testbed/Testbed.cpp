#include "Testbed.h"

#include <Vulk/Device.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/DepthImage.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/Queue.h>
#include <Vulk/Exception.h>

#include <Vulk/engine/Toolbox.h>
#include <Vulk/internal/debug.h>

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
#include <string>
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

Testbed::ValidationLevel Testbed::_validationLevel = ValidationLevel::Error;

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
std::filesystem::path executablePath() {
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

struct Uniforms {
  alignas(sizeof(glm::vec4)) glm::mat4 model;
  alignas(sizeof(glm::vec4)) glm::mat4 view;
  alignas(sizeof(glm::vec4)) glm::mat4 proj;

  static Vulk::UniformBuffer::shared_ptr allocateBuffer(const Vulk::Device& device) {
    return Vulk::UniformBuffer::make_shared(device, sizeof(Uniforms));
  }

  static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
    return {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
  }
};
} // namespace

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createContext();
  createDrawable();
  createRenderTask();
  createFrames();

  _zoomFactor = 1.0F;
}

void Testbed::cleanup() {
  for (auto& frame : _frames) {
    frame.colorBuffer->destroy();
    frame.depthBuffer->destroy();
  }
  _frames.clear();

  _acquireSwapchainImageTask.reset();
  _textureMappingTask.reset();
  _presentTask.reset();

  _texture->destroy();
  _drawable.destroy();

  _context.destroy();

  MainWindow::cleanup();
}

void Testbed::run() {
  MainWindow::run();
}

void Testbed::mainLoop() {
  MainWindow::mainLoop();
  _context.waitIdle(); // Before we end the loop, we need to wait for the device to be idle.
}

void Testbed::drawFrame() {
  const auto& commandPool = _context.commandPool(Vulk::Device::QueueFamilyType::Graphics);
  auto commandBuffer      = Vulk::CommandBuffer::make_shared(commandPool);

  try {
    nextFrame(); // Move to the next frame. The new current frame may be in use.

    auto label = _currentFrame->commandBuffer->queue().scopedLabel("Testbed::drawFrame()");

    _acquireSwapchainImageTask->prepareSynchronization(_currentFrame->swapchainImageReady.get());
    _acquireSwapchainImageTask->run(*commandBuffer);

    //
    // Texture Mapping Task
    //
    _textureMappingTask->prepareGeometry(
        _drawable.vertexBuffer(), _drawable.indexBuffer(), _drawable.numIndices());
    _textureMappingTask->prepareUniforms(
        glm::mat4{1.0F}, _camera->viewMatrix(), _camera->projectionMatrix());
    _textureMappingTask->prepareInputs(*_texture);
    _textureMappingTask->prepareOutputs(*_currentFrame->colorBuffer, *_currentFrame->depthBuffer);
    _textureMappingTask->prepareSynchronization({}, {_currentFrame->frameReady.get()});

    _textureMappingTask->run(*commandBuffer);

    //
    // Present Task
    //
    _presentTask->prepareInput(*_currentFrame->colorBuffer);
    _presentTask->prepareSynchronization(
        {_currentFrame->swapchainImageReady.get(), _currentFrame->frameReady.get()},
        {_currentFrame->presentReady.get()});

    _presentTask->run(*commandBuffer);
  } catch (const Vulk::Exception& e) {
    if (e.result() == VK_ERROR_OUT_OF_DATE_KHR || e.result() == VK_SUBOPTIMAL_KHR) {
      // The size or format of the swapchain image is not correct. We'll just ignore them
      // and let the resize callback to recreate the swapchain.
    } else {
      throw e;
    }
  }

  commandBuffer->queue().waitIdle(); //TODO fix this hack
}

void Testbed::createContext() {
  Vulk::Context::CreateInfo createInfo;
  createInfo.versionMajor       = 1;
  createInfo.versionMinor       = 0;
  createInfo.instanceExtensions = getRequiredInstanceExtensions();
  createInfo.validationLevel    = _validationLevel;

  createInfo.createWindowSurface = [this](const Vulk::Instance& instance) {
    return MainWindow::createWindowSurface(instance);
  };

  createInfo.queueFamilies.graphics    = true;
  createInfo.queueFamilies.present     = true;
  createInfo.deviceExtensions          = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  createInfo.hasPhysicalDeviceFeatures = [](VkPhysicalDeviceFeatures supportedFeatures) {
    return supportedFeatures.samplerAnisotropy != 0U;
  };

  createInfo.chooseSurfaceFormat = &Testbed::chooseSwapchainSurfaceFormat;
  createInfo.chooseSurfaceExtent = [this](const VkSurfaceCapabilitiesKHR& caps) {
    return chooseSwapchainSurfaceExtent(caps, width(), height());
  };
  createInfo.choosePresentMode = &Testbed::chooseSwapchainPresentMode;

  _context.create(createInfo);
}

void Testbed::createRenderTask() {
  _acquireSwapchainImageTask = Vulk::AcquireSwapchainImageTask::make_shared(_context);
  _textureMappingTask        = Vulk::TextureMappingTask::make_shared(_context);
  _presentTask               = Vulk::PresentTask::make_shared(_context);
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

void Testbed::initCamera(const std::vector<Vertex>& vertices, bool is3D) {
  auto bbox = Vulk::ArcCamera::BBox::null();
  for (const auto& vertex : vertices) {
    bbox += vertex.pos;
  }
  bbox.expandPlanarSide(1.0F);

  auto extent = _context.swapchain().surfaceExtent();
  if (is3D) {
    _camera = Vulk::ArcCamera::make_shared(glm::vec2{extent.width, extent.height}, bbox);
  } else {
    _camera = Vulk::FlatCamera::make_shared(glm::vec2{extent.width, extent.height}, bbox);
  }
}

void Testbed::createDrawable() {
  if (_textureFile.empty()) {
    const glm::uvec2 numBlocks{4, 4};
    const glm::uvec2 blockSize{128, 128};
    const glm::uvec4 black{60, 60, 60, 255};
    const glm::uvec4 white{255, 255, 255, 255};
    Checkerboard checkerboard{numBlocks, blockSize, black, white};
    _texture = Vulk::Toolbox(_context).createTexture2D(Vulk::Toolbox::TextureFormat::RGBA,
                                                       checkerboard.data(),
                                                       checkerboard.extent.x,
                                                       checkerboard.extent.y);

    VkImage image = *_texture;
    _context.device().setObjectName(
        VK_OBJECT_TYPE_IMAGE, (uint64_t)image, "Created texture (checkerboard)");
  } else {
    _texture = Vulk::Toolbox(_context).createTexture2D(_textureFile.c_str());

    std::string name;
    name          = "Loaded texture (" + _textureFile + ")";
    VkImage image = *_texture;
    _context.device().setObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)image, name.c_str());
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  if (_modelFile.empty()) {
    float left{-1.0F};
    float right{1.0F};
    float bottom{-1.0F};
    float top{1.0F};

    if (_texture->isValid()) {
      // to make sure the texture and the quad has the same aspect ratio
      auto [textureW, textureH] = _texture->extent();
      const float textureAspect = static_cast<float>(textureW) / static_cast<float>(textureH);

      if (textureAspect > 1.0F) {
        left  = -textureAspect;
        right = textureAspect;
      } else {
        bottom = -1.0F / textureAspect;
        top    = 1.0F / textureAspect;
      }
    }
    vertices = {{{left, bottom, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
                {{left, top, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
                {{right, top, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
                {{right, bottom, 0.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}}};
    indices  = {0, 1, 2, 2, 3, 0};

  } else {
    loadModel(_modelFile, vertices, indices);
  }
  Vulk::CommandBuffer commandBuffer{_context.commandPool(Vulk::Device::QueueFamilyType::Graphics)}; // TODO: should't we use `Transfer`?
  _drawable.create(_context.device(), commandBuffer, vertices, indices);

  const bool is3DScene = !_modelFile.empty();
  initCamera(vertices, is3DScene);
}

void Testbed::createFrames() {
  _frames.resize(_maxFramesInFlight);

  const auto& device = _context.device();
  const auto& extent = _context.swapchain().surfaceExtent();

  const auto& cmdPool = _context.commandPool(Vulk::Device::QueueFamilyType::Graphics);
  for (auto& frame : _frames) {
    frame.commandBuffer = Vulk::CommandBuffer::make_shared(cmdPool);

    const auto usage  = Vulk::Image2D::Usage::COLOR_ATTACHMENT | Vulk::Image2D::Usage::TRANSFER_SRC;
    frame.colorBuffer = Vulk::Image2D::make_shared(device, VK_FORMAT_B8G8R8A8_SRGB, extent, usage);
    frame.colorBuffer->allocate();
    frame.depthBuffer = Vulk::DepthImage::make_shared(device, extent, chooseDepthFormat());
    frame.depthBuffer->allocate();

    frame.swapchainImageReady = Vulk::Semaphore::make_shared(device);
    frame.frameReady          = Vulk::Semaphore::make_shared(device);
    frame.presentReady        = Vulk::Semaphore::make_shared(device);

    frame.colorBuffer->transitToNewLayout(*frame.commandBuffer,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }

  nextFrame();
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
  _camera->update(glm::vec2{extent.width, extent.height});

  setFramebufferResized(false);

  // To force a drawFrame()
  MainWindow::postEmptyEvent();
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
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFramesInFlight;
  _currentFrame    = &_frames[_currentFrameIdx];
}

void Testbed::onKeyInput(int key, int action, int mods) {
  MainWindow::onKeyInput(key, action, mods);

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_EQUAL && mods == GLFW_MOD_CONTROL) {
      _camera->zoom(_zoomFactor = 1.0F);
    } else if (key == GLFW_KEY_UP && mods == GLFW_MOD_CONTROL) {
      _camera->orbitVertical(M_PI / 2.0F);
    } else if (key == GLFW_KEY_DOWN && mods == GLFW_MOD_CONTROL) {
      _camera->orbitVertical(-M_PI / 2.0F);
    } else if (key == GLFW_KEY_RIGHT && mods == GLFW_MOD_CONTROL) {
      _camera->orbitHorizontal(M_PI / 2.0F);
    } else if (key == GLFW_KEY_LEFT && mods == GLFW_MOD_CONTROL) {
      _camera->orbitHorizontal(-M_PI / 2.0F);
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
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    _camera->move(mousePosHistory.front(), {xpos, ypos});
    mousePosHistory.pop();
    mousePosHistory.push({xpos, ypos});
  } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (mousePosHistory.size() >= 2) {
      _camera->rotate(mousePosHistory.front(), {xpos, ypos});
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
    _camera->zoom(_zoomFactor *= (1.0F + delta));
  } else {
    _camera->zoom(_zoomFactor *= (1.0F - delta));
  }
}

void Testbed::onFramebufferResize(int width, int height) {
  MainWindow::onFramebufferResize(width, height);
  resizeSwapchain();
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
