#pragma once

#include <Vulk/Image2D.h>
#include <Vulk/Texture2D.h>
#include <Vulk/helpers_vulkan.h>

#include <tuple>

NAMESPACE_BEGIN(Vulk)

class Context;

class Toolbox {
 public:
  Toolbox(const Context& context);
  ~Toolbox() = default;

  Image2D createImage2D(const char* imageFile) const;
  Texture2D createTexture(const char* textureFile) const;

  Toolbox(const Toolbox& rhs)            = delete;
  Toolbox& operator=(const Toolbox& rhs) = delete;

 private:
  using width_t  = uint32_t;
  using height_t = uint32_t;
  std::tuple<StagingBuffer, width_t, height_t> createStagingBuffer(const char* imageFile) const;

 private:
  const Vulk::Context& _context;
};

NAMESPACE_END(Vulk)