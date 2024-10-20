#include "Testbed.h"

#include "apps/ModelViewer.h"
#include "apps/ImageViewer.h"

#include <GLFW/glfw3.h>

#include <queue>
#include <iostream>
#include <filesystem>

#include <Vulk/internal/debug.h>

Testbed::ValidationLevel Testbed::_validationLevel = ValidationLevel::None;
bool Testbed::_debugUtilsEnabled                   = false;

void Testbed::setVulkanValidationLevel(ValidationLevel level) {
  _validationLevel = level;
}
void Testbed::setVulkanDebugUtilsExt(bool enable) {
  _debugUtilsEnabled = enable;
}
void Testbed::setPrintSpirvReflect(bool print) {
  print ? Vulk::ShaderModule::enablePrintReflection()
        : Vulk::ShaderModule::disablePrintReflection();
}

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createDeviceContext();

  if (!_app) {
    MI_LOG_INFO("No app selected. Default to [ModelViewer].");
    _app = App::registry().get(ModelViewer::ID);
  }

  App::Params params;
  params.add(App::PARAM_MODEL_FILE, _modelFile);
  params.add(App::PARAM_TEXTURE_FILE, _textureFile);
  _app->init(_deviceContext, params);

  _zoomFactor = 1.0F;

  // After we init all Vulkan resource and before the rendering, make sure the/ device is idle and
  // all resource is ready.
  _deviceContext->waitIdle();
}

void Testbed::cleanup() {
  // Before we clean up all Vulkan resource, make sure the device is idle.
  _deviceContext->waitIdle();

  _app->cleanup();

  _deviceContext->destroy();

  MainWindow::cleanup();
}

void Testbed::run() {
  MainWindow::run();
}

void Testbed::mainLoop() {
  MainWindow::mainLoop();
}

void Testbed::drawFrame() {
  _app->render();
}

void Testbed::createDeviceContext() {
  Vulk::DeviceContext::CreateInfo createInfo;
  createInfo.versionMajor       = 1;
  createInfo.versionMinor       = 0;
  createInfo.instanceExtensions = getRequiredInstanceExtensions();
  if (_debugUtilsEnabled) {
    createInfo.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  createInfo.validationLevel = _validationLevel;

  createInfo.createWindowSurface = [this](const Vulk::Instance& instance) {
    return MainWindow::createWindowSurface(instance);
  };

  createInfo.queueFamilies.graphics = true;
  // createInfo.queueFamilies.compute     = true;
  createInfo.queueFamilies.transfer = true;
  createInfo.queueFamilies.present  = true;

  createInfo.deviceExtensions          = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  createInfo.hasPhysicalDeviceFeatures = [](VkPhysicalDeviceFeatures supportedFeatures) {
    return supportedFeatures.samplerAnisotropy != 0U;
  };

  createInfo.chooseSurfaceFormat = &Testbed::chooseSwapchainSurfaceFormat;
  createInfo.chooseSurfaceExtent = [this](const VkSurfaceCapabilitiesKHR& caps) {
    return chooseSwapchainSurfaceExtent(caps, width(), height());
  };
  createInfo.choosePresentMode = &Testbed::chooseSwapchainPresentMode;

  _deviceContext = Vulk::DeviceContext::make_shared();
  _deviceContext->create(createInfo);
}

void Testbed::resizeSwapchain() {
  if (isMinimized()) {
    return;
  }
  _deviceContext->waitIdle();
  _deviceContext->swapchain().resize(width(), height());

  // We need to get the updated size from the surface directly. It is not guaranteed that the extent
  // is the same as the {width(), height()}.
  auto extent = _deviceContext->swapchain().surfaceExtent();
  _app->resize(extent.width, extent.height);

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

void Testbed::onKeyInput(int key, int action, int mods) {
  MainWindow::onKeyInput(key, action, mods);

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_EQUAL && mods == GLFW_MOD_CONTROL) {
      _app->camera().zoom(_zoomFactor = 1.0F);
    } else if (key == GLFW_KEY_UP && mods == GLFW_MOD_CONTROL) {
      _app->camera().orbitVertical(M_PI / 2.0F);
    } else if (key == GLFW_KEY_DOWN && mods == GLFW_MOD_CONTROL) {
      _app->camera().orbitVertical(-M_PI / 2.0F);
    } else if (key == GLFW_KEY_RIGHT && mods == GLFW_MOD_CONTROL) {
      _app->camera().orbitHorizontal(M_PI / 2.0F);
    } else if (key == GLFW_KEY_LEFT && mods == GLFW_MOD_CONTROL) {
      _app->camera().orbitHorizontal(-M_PI / 2.0F);
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
    _app->camera().move(mousePosHistory.front(), {xpos, ypos});
    mousePosHistory.pop();
    mousePosHistory.push({xpos, ypos});
  } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (mousePosHistory.size() >= 2) {
      _app->camera().rotate(mousePosHistory.front(), {xpos, ypos});
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
    _app->camera().zoom(_zoomFactor *= (1.0F + delta));
  } else {
    _app->camera().zoom(_zoomFactor *= (1.0F - delta));
  }
}

void Testbed::onFramebufferResize(int width, int height) {
  MainWindow::onFramebufferResize(width, height);
  resizeSwapchain();
}

void Testbed::setApp(const std::string& appName) {
  _app = App::registry().get(appName);
  if (_app) {
    MI_LOG_INFO("App [%s] selected.", appName.c_str());
  } else {
    MI_LOG_WARNING("App [%s] not found! Will default to [ModelViewer].", appName.c_str());
  }
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

std::filesystem::path locateFile(const std::string& file) {
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
  MI_LOG_INFO("Model file: %s", _modelFile.c_str());
}
void Testbed::setTextureFile(const std::string& textureFile) {
  _textureFile = locateFile(textureFile);
  if (_textureFile.empty()) {
    throw std::runtime_error("Error: texture file [" + textureFile + "] does not exist!");
  }

  MI_LOG_INFO("Texture file: %s", _textureFile.c_str());
}
