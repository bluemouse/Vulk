#pragma once

#include <Vulk/internal/base.h>

#include <Vulk/Image2D.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/engine/Texture2D.h>

#include <tuple>

NAMESPACE_BEGIN(Vulk)

class Context;

class Toolbox {
 public:
  Toolbox(const Context& context);
  ~Toolbox() = default;

  Image2D::shared_ptr createImage2D(const char* imageFile) const;
  Texture2D::shared_ptr createTexture2D(const char* textureFile) const;

  enum class TextureFormat { RGB, RGBA };
  Texture2D::shared_ptr createTexture2D(TextureFormat format,
                                        const uint8_t* data,
                                        uint32_t width,
                                        uint32_t height) const;

  Toolbox(const Toolbox& rhs)            = delete;
  Toolbox& operator=(const Toolbox& rhs) = delete;

 private:
  using width_t  = uint32_t;
  using height_t = uint32_t;
  std::tuple<StagingBuffer::shared_ptr, width_t, height_t> createStagingBuffer(const char* imageFile) const;
  StagingBuffer::shared_ptr createStagingBuffer(const uint8_t* data, uint32_t size) const;

 private:
  const Vulk::Context& _context;
};

NAMESPACE_END(Vulk)