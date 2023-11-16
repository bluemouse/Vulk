#pragma once

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/StagingBuffer.h>

#include <vector>

NAMESPACE_BEGIN(Vulk)

template <typename Vertex, typename Index>
class Drawable {
 public:
  using vertex_type = Vertex;
  using index_type  = Index;

 public:
  Drawable() = default;

  void create(const Vulk::Device& device,
              const Vulk::CommandBuffer& commandBuffer,
              std::vector<vertex_type> vertices,
              std::vector<index_type> indices);
  void destroy();

  [[nodiscard]] const Vulk::VertexBuffer& vertexBuffer() const { return _vertexBuffer; }
  [[nodiscard]] const Vulk::IndexBuffer& indexBuffer() const { return _indexBuffer; }

  [[nodiscard]] size_t numIndices() const { return _numIndices; }

 private:
  Vulk::VertexBuffer _vertexBuffer;
  Vulk::IndexBuffer _indexBuffer;

  size_t _numIndices = 0;
};

template <typename V, typename I>
inline void Drawable<V, I>::create(const Vulk::Device& device,
                                   const Vulk::CommandBuffer& commandBuffer,
                                   std::vector<vertex_type> vertices,
                                   std::vector<index_type> indices) {
  _vertexBuffer.create(device, commandBuffer, vertices);
  _indexBuffer.create(device, commandBuffer, indices);
  _numIndices = indices.size();
}

template <typename V, typename I>
inline void Drawable<V, I>::destroy() {
  _vertexBuffer.destroy();
  _indexBuffer.destroy();

  _numIndices = 0;
}

NAMESPACE_END(Vulk)