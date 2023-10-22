#pragma once

#include <Vulk/Image.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

class Context;

class Toolbox {
 public:
  Toolbox(const Context& context);
  ~Toolbox() = default;

  Image createImage(const char* imageFile) const;

  Toolbox(const Toolbox& rhs) = delete;
  Toolbox& operator=(const Toolbox& rhs) = delete;

 private:
  const Vulk::Context& _context;
};

NAMESPACE_Vulk_END