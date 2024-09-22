#pragma once

#include <Vulk/internal/base.h>
#include <Vulk/engine/Bound.h>

#include <glm/glm.hpp>

NAMESPACE_BEGIN(Vulk)

class Camera : public Sharable<Camera> {
 public:
  using BBox = Bound<glm::vec3>;

 public:
  Camera() = default;
  virtual ~Camera() = default;

  // Update the camera matrices with new frame size.
  virtual void update(const glm::vec2& frameSize) = 0;

  virtual void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) = 0;
  virtual void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) = 0;
  virtual void zoom(float scale) = 0;

  [[nodiscard]] glm::mat4 viewMatrix() const { return _world2View; }
  [[nodiscard]] glm::mat4 projectionMatrix() const { return _projection; }

  [[nodiscard]] float frameWidth() const { return _frameSize.x; }
  [[nodiscard]] float frameHeight() const { return _frameSize.y; }

  // Coordinate conversion functions
  [[nodiscard]] virtual glm::vec3 screen2view(glm::vec2 p) const = 0;
  [[nodiscard]] virtual glm::vec3 screen2world(glm::vec2 p) const = 0;
  [[nodiscard]] virtual glm::vec2 screen2ndc(glm::vec2 p) const = 0;
  [[nodiscard]] virtual glm::vec3 ndc2view(glm::vec3 p) const = 0;
  [[nodiscard]] virtual glm::vec3 ndc2world(glm::vec3 p) const = 0;
  [[nodiscard]] virtual glm::vec3 view2world(glm::vec3 p) const = 0;

 protected:
  void setFrameSize(glm::vec2 size) { _frameSize = size; }

 protected:
  BBox _roi;            // region-of-interest bounding box in world space
  glm::vec2 _frameSize; // Frame size in pixels

  BBox _viewVolume; // view volume in view space
  float _zoomScale{1.0F};

  glm::mat4 _world2View, _view2World;
  glm::mat4 _projection, _invProjection;
};

class ArcCamera : public Camera {
 public:
  ArcCamera() : ArcCamera{glm::vec2{1.0f}, BBox::unit()} {}
  ArcCamera(const glm::vec2& frameSize, const BBox& roi) : Camera{} { init(frameSize, roi); }

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
  void update(const glm::vec2& frameSize) override;

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) override;
  void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) override;
  void zoom(float scale) override;

  // Orbit the camera around `center` by `angle` radians along `axis`. `axis` is in world space.
  void orbit(const glm::vec3& axis, float angle);
  // Orbit the camera around `center` by `angle` radians along the view space horizontal axis.
  void orbitHorizontal(float angle);
  // Orbit the camera around `center` by `angle` radians along the view space vertical axis.
  void orbitVertical(float angle);

  // Coordinate conversion functions
  [[nodiscard]] glm::vec3 screen2view(glm::vec2 p) const override;
  [[nodiscard]] glm::vec3 screen2world(glm::vec2 p) const override;
  [[nodiscard]] glm::vec2 screen2ndc(glm::vec2 p) const override;
  [[nodiscard]] glm::vec3 ndc2view(glm::vec3 p) const override;
  [[nodiscard]] glm::vec3 ndc2world(glm::vec3 p) const override;
  [[nodiscard]] glm::vec3 view2world(glm::vec3 p) const override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(ArcCamera, Camera);

 private:
  // Update the camera matrices.
  void update();

  struct Rotation {
    glm::vec3 axis;
    float angle;
  };
  [[nodiscard]] Rotation computeTrackballRotation(const glm::vec2& screenFrom,
                                                  const glm::vec2& screenTo) const;
  [[nodiscard]] glm::vec3 trackballPoint(glm::vec2 screenPos) const;

 private:
  // Camera parameters in world space
  glm::vec3 _eye;
  glm::vec3 _lookAt;
  glm::vec3 _up;
};

NAMESPACE_END(Vulk)