#pragma once

#include <volk/volk.h>

#include <stdexcept>

MI_NAMESPACE_BEGIN(Vulk)

class Exception : public std::logic_error {
 public:
  Exception(VkResult result, const std::string& info) : std::logic_error(info), _result{result} {}

  VkResult result() const { return _result; }

 private:
  VkResult _result;
};

MI_NAMESPACE_END(Vulk)