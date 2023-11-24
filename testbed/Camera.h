#pragma once

#include "Bound.h"

#include <glm/glm.hpp>

#include <limits>

class Camera {
 public:
  using BBox = Bound<glm::vec3>;

 public:
  Camera() { init(); }

  // Camera, located at 2x radius of `roi`, looks at center of `roi` toward positive z.  Camera up
  // vector is negative y initially. `frameSize` is the dimension of the frame buffer. It is used
  // for fitting and other frame-size-dependent calculations.
  void init(const BBox& roi, const glm::vec2& frameSize);
  // Update the camera matrices with new frame size.
  void update(const glm::vec2& frameSize);
  // Update the camera matrices.
  void update();
  void reset();

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition);
  void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition);
  void zoom(float scale);

  [[nodiscard]] glm::mat4 mvpMatrix() const { return _mvp; }
  [[nodiscard]] glm::mat4 modelMatrix() const { return _model2World; }
  [[nodiscard]] glm::mat4 viewMatrix() const { return _world2View; }
  [[nodiscard]] glm::mat4 projectionMatrix() const { return _projection; }

  [[nodiscard]] float frameWidth() const { return _frameSize.x; }
  [[nodiscard]] float frameHeight() const { return _frameSize.y; }

  // Coordinate conversion functions
  [[nodiscard]] glm::vec3 screen2view(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 screen2world(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 screen2model(glm::vec2 p) const;
  [[nodiscard]] glm::vec2 screen2ndc(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 ndc2view(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 ndc2world(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 view2world(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 world2model(glm::vec3 p) const;

 private:
  void init() { init(BBox::unit(), glm::vec2{1.0f}); }

  void setFrameSize(glm::vec2 size) { _frameSize = size; }

  [[nodiscard]] glm::vec3 trackballPoint(const glm::vec2& screenPos) const;

 private:
  // Camera parameters in world space
  glm::vec3 _eye;
  glm::vec3 _lookAt;
  glm::vec3 _up;

  BBox _roi; // region-of-interest bounding box in world space
  glm::vec2 _frameSize; // Frame size in pixels

  BBox _viewVolume; // view volume in view space
  float _zoomScale{1.0F};

  glm::mat4 _model2World, _world2Model;
  glm::mat4 _world2View, _view2World;
  glm::mat4 _projection, _invProjection;
  glm::mat4 _mvp;
};
