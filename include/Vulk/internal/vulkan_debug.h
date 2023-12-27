#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/debug.h>

#define MI_VERIFY_VKCMD(cmd)                                             \
  if (cmd != VK_SUCCESS) {                                               \
    throw std::runtime_error("Error: " #cmd " failed" _MI_AT_THIS_LINE); \
  }
#define MI_VERIFY_VKCMD_MSG(cmd, msg)               \
  if (cmd != VK_SUCCESS) {                          \
    throw std::runtime_error(msg _MI_AT_THIS_LINE); \
  }

#define MI_VERIFY_VKHANDLE(handle)                                           \
  if (handle == VK_NULL_HANDLE) {                                            \
    throw std::runtime_error("Error: " #handle " is null" _MI_AT_THIS_LINE); \
  }
#define MI_VERIFY_VKHANDLE_MSG(handle, msg)         \
  if (handle == VK_NULL_HANDLE) {                   \
    throw std::runtime_error(msg _MI_AT_THIS_LINE); \
  }
