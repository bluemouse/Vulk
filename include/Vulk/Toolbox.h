#pragma once

#include <Vulk/Image.h>
#include <Vulk/Texture.h>
#include <Vulk/helpers_vulkan.h>

#include <tuple>

NAMESPACE_Vulk_BEGIN

class Context;

class Toolbox {
 public:
  Toolbox(const Context& context);
  ~Toolbox() = default;

  Image createImage(const char* imageFile) const;
  Texture createTexture(const char* textureFile) const;

  Toolbox(const Toolbox& rhs) = delete;
  Toolbox& operator=(const Toolbox& rhs) = delete;

private:
  using width_t = uint32_t;
  using height_t = uint32_t;
  std::tuple<StagingBuffer, width_t, height_t> createStagingBuffer(const char* imageFile) const;

 private:
  const Vulk::Context& _context;
};

NAMESPACE_Vulk_END