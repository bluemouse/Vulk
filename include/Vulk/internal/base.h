#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/debug.h>

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END(name) }

#define MI_INIT_VKPROC(cmd)                                                       \
  auto cmd = reinterpret_cast<PFN_##cmd>(vkGetInstanceProcAddr(_instance, #cmd)); \
  MI_VERIFY(cmd != nullptr);
