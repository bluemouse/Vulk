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
  void init(const glm::vec2& frameSize, const BBox& roi);
  void init(const glm::vec2& frameSize,
            const BBox& roi,
            const glm::vec3& up,
            const glm::vec3& eyeRay,
            float zoomScale = 1.0F);
  // Update the camera matrices with new frame size.
  void update(const glm::vec2& frameSize);
  // Update the camera matrices.
  void update();

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition);
  void rotate(const glm::vec2& origScreenPosition,
              const glm::vec2& fromScreenPosition,
              const glm::vec2& toScreenPosition);
  void zoom(float scale);

  // Orbit the camera around `center` by `angle` radians along `axis`. `axis` is in world space.
  void orbit(const glm::vec3& axis, float angle);
  // Orbit the camera around `center` by `angle` radians along the view space horizontal axis.
  void orbitHorizontal(float angle);
  // Orbit the camera around `center` by `angle` radians along the view space vertical axis.
  void orbitVertical(float angle);

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
  void init() { init(glm::vec2{1.0f}, BBox::unit()); }

  void setFrameSize(glm::vec2 size) { _frameSize = size; }

  struct Rotation {
    glm::vec3 axis;
    float angle;
  };
  [[nodiscard]] Rotation computeTrackballRotation(const glm::vec2& screenOrigin,
                                                  const glm::vec2& screenFrom,
                                                  const glm::vec2& screenTo) const;
  [[nodiscard]] glm::vec3 trackballPoint(glm::vec2 screenPos) const;

 private:
  // Camera parameters in world space
  glm::vec3 _eye;
  glm::vec3 _lookAt;
  glm::vec3 _up;

  BBox _roi;            // region-of-interest bounding box in world space
  glm::vec2 _frameSize; // Frame size in pixels

  BBox _viewVolume; // view volume in view space
  float _zoomScale{1.0F};

  glm::mat4 _model2World, _world2Model;
  glm::mat4 _world2View, _view2World;
  glm::mat4 _projection, _invProjection;
  glm::mat4 _mvp;
};
