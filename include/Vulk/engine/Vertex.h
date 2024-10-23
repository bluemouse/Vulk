#pragma once

#include <Vulk/engine/TypeTraits.h>

#include <vector>

MI_NAMESPACE_BEGIN(Vulk)

template <typename Position, typename Color, typename TexCoord>
struct VertexPCT {
  using position_type = Position;
  using color_type    = Color;
  using texcoord_type = TexCoord;

  position_type pos;
  color_type color;
  texcoord_type texCoord;

  bool operator==(const VertexPCT& rhs) const {
    return pos == rhs.pos && color == rhs.color && texCoord == rhs.texCoord;
  }

  static VkVertexInputBindingDescription bindingDescription(uint32_t binding) {
    return {binding, VertexPCT::size(), VK_VERTEX_INPUT_RATE_VERTEX};
  }
  static std::vector<VkVertexInputAttributeDescription> attributesDescription(uint32_t binding) {
    // In shader.vert, we should have:
    // layout(location = 0) in vec3 inPos;
    // layout(location = 1) in vec3 inColor;
    // layout(location = 2) in vec2 inTexCoord;
    return {{0, binding, formatof(VertexPCT::pos), offsetof(VertexPCT, pos)},
            {1, binding, formatof(VertexPCT::color), offsetof(VertexPCT, color)},
            {2, binding, formatof(VertexPCT::texCoord), offsetof(VertexPCT, texCoord)}};
  }
  static constexpr uint32_t size() { return sizeof(VertexPCT); }
};

template <typename Position, typename Color>
struct VertexPC {
  using position_type = Position;
  using color_type    = Color;

  position_type pos;
  color_type color;

  bool operator==(const VertexPC& rhs) const {
    return pos == rhs.pos && color == rhs.color;
  }

  static VkVertexInputBindingDescription bindingDescription(uint32_t binding) {
    return {binding, VertexPC::size(), VK_VERTEX_INPUT_RATE_VERTEX};
  }
  static std::vector<VkVertexInputAttributeDescription> attributesDescription(uint32_t binding) {
    // In shader.vert, we should have:
    // layout(location = 0) in vec3 inPos;
    // layout(location = 1) in vec3 inColor;
    return {{0, binding, formatof(VertexPC::pos), offsetof(VertexPC, pos)},
            {1, binding, formatof(VertexPC::color), offsetof(VertexPC, color)}};
  }
  static constexpr uint32_t size() { return sizeof(VertexPC); }
};

MI_NAMESPACE_END(Vulk)