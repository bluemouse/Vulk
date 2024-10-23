#pragma once

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/StagingBuffer.h>

#include <vector>

MI_NAMESPACE_BEGIN(Vulk)

class Drawable {
 public:
  Drawable() = default;

  virtual void destroy() = 0;
};

//
// Mesh
//
template <typename Vertex, typename Index>
class MeshDrawable : public Drawable {
 public:
  using vertex_type = Vertex;
  using index_type  = Index;

 public:
  MeshDrawable() = default;

  void create(const Device& device,
              const std::vector<vertex_type>& vertices,
              const std::vector<index_type>& indices);
  void destroy() override;

  [[nodiscard]] const VertexBuffer& vertexBuffer() const { return *_vertexBuffer; }
  [[nodiscard]] const IndexBuffer& indexBuffer() const { return *_indexBuffer; }

  [[nodiscard]] size_t numVertices() const { return _numVertices; }
  [[nodiscard]] size_t numIndices() const { return _numIndices; }

 private:
  VertexBuffer::shared_ptr _vertexBuffer;
  IndexBuffer::shared_ptr _indexBuffer;

  size_t _numVertices = 0;
  size_t _numIndices  = 0;
};

template <typename V, typename I>
inline void MeshDrawable<V, I>::create(const Device& device,
                                       const std::vector<vertex_type>& vertices,
                                       const std::vector<index_type>& indices) {
  _vertexBuffer = VertexBuffer::make_shared(device, vertices);
  _indexBuffer  = IndexBuffer::make_shared(device, indices);

  _numVertices = vertices.size();
  _numIndices  = indices.size();
}

template <typename V, typename I>
inline void MeshDrawable<V, I>::destroy() {
  _vertexBuffer.reset();
  _indexBuffer.reset();

  _numVertices = 0;
  _numIndices  = 0;
}

//
// Points
//
template <typename Vertex>
class PointsDrawable : public Drawable {
 public:
  using vertex_type = Vertex;

 public:
  PointsDrawable() = default;

  void create(const Device& device, const std::vector<vertex_type>& vertices);
  void destroy() override;

  void update(const std::vector<vertex_type>& vertices);

  [[nodiscard]] const VertexBuffer& vertexBuffer() const { return *_vertexBuffer; }

  [[nodiscard]] size_t numVertices() const { return _numVertices; }

 private:
  VertexBuffer::shared_ptr _vertexBuffer;

  size_t _numVertices = 0;
};

template <typename V>
inline void PointsDrawable<V>::create(const Device& device,
                                      const std::vector<vertex_type>& vertices) {
  _vertexBuffer = VertexBuffer::make_shared(device, vertices);
  _numVertices  = vertices.size();
}

template <typename V>
inline void PointsDrawable<V>::destroy() {
  _vertexBuffer.reset();
  _numVertices = 0;
}

template <typename V>
inline void PointsDrawable<V>::update(const std::vector<vertex_type>& vertices) {
  _vertexBuffer->update(vertices);
}

MI_NAMESPACE_END(Vulk)