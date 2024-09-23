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

  // Camera, orthographic and located at 2x radius of `roi`, looks at center of `roi`
  // toward positive z.  Camera up vector is negative y initially. `frameSize` is the
  // dimension of the frame buffer. It is used for fitting and other frame-size-dependent
  // calculations.
  void init(const glm::vec2& frameSize,
            const BBox& roi,
            const glm::vec3& up,
            const glm::vec3& eyeRay,
            float zoomScale = 1.0F);

  // Update the camera matrices with new frame size.
  void update(const glm::vec2& frameSize);

  virtual void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) = 0;
  virtual void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) = 0;
  virtual void zoom(float scale) = 0;

  // Orbit the camera around `center` by `angle` radians along `axis`. `axis` is in world space.
  virtual void orbit(const glm::vec3& axis, float angle) = 0;
  // Orbit the camera around `center` by `angle` radians along the view space horizontal axis.
  virtual void orbitHorizontal(float angle) = 0;
  // Orbit the camera around `center` by `angle` radians along the view space vertical axis.
  virtual void orbitVertical(float angle) = 0;


  [[nodiscard]] glm::mat4 viewMatrix() const { return _world2View; }
  [[nodiscard]] glm::mat4 projectionMatrix() const { return _projection; }

  [[nodiscard]] float frameWidth() const { return _frameSize.x; }
  [[nodiscard]] float frameHeight() const { return _frameSize.y; }

  // Coordinate conversion functions
  [[nodiscard]] glm::vec3 screen2view(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 screen2world(glm::vec2 p) const;
  [[nodiscard]] glm::vec2 screen2ndc(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 ndc2view(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 ndc2world(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 view2world(glm::vec3 p) const;

 protected:
  // Update the camera matrices.
  void update();

  void setFrameSize(glm::vec2 size) { _frameSize = size; }

 protected:
  // Camera parameters in world space
  glm::vec3 _eye;
  glm::vec3 _lookAt;
  glm::vec3 _up;

  // region-of-interest bounding box in world space
  BBox _roi;
  // Frame size in pixels
  glm::vec2 _frameSize;

  // view volume in view space
  BBox _viewVolume;
  float _zoomScale{1.0F};

  glm::mat4 _world2View, _view2World;
  glm::mat4 _projection, _invProjection;
};

class ArcCamera : public Camera {
 public:
  ArcCamera() : ArcCamera{glm::vec2{1.0f}, BBox::unit()} {}
  ArcCamera(const glm::vec2& frameSize, const BBox& roi) : Camera{} { init(frameSize, roi); }

  using Camera::init;
  void init(const glm::vec2& frameSize, const BBox& roi);

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) override;
  void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) override;
  void zoom(float scale) override;

  // Orbit the camera around `center` by `angle` radians along `axis`. `axis` is in world space.
  void orbit(const glm::vec3& axis, float angle) override;
  // Orbit the camera around `center` by `angle` radians along the view space horizontal axis.
  void orbitHorizontal(float angle) override;
  // Orbit the camera around `center` by `angle` radians along the view space vertical axis.
  void orbitVertical(float angle) override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(ArcCamera, Camera);

 private:
  struct Rotation {
    glm::vec3 axis;
    float angle;
  };
  [[nodiscard]] Rotation computeTrackballRotation(const glm::vec2& screenFrom,
                                                  const glm::vec2& screenTo) const;
  [[nodiscard]] glm::vec3 trackballPoint(glm::vec2 screenPos) const;
};

// A 2D camera to render a plane in 3D space. The camera is orthographic toward the x-y plane of
// the renderable.
class FlatCamera : public Camera {
 public:
  FlatCamera() : FlatCamera{glm::vec2{1.0f}, BBox::unit()} {}
  FlatCamera(const glm::vec2& frameSize, const BBox& roi) : Camera{} { init(frameSize, roi); }

  using Camera::init;
  void init(const glm::vec2& frameSize, const BBox& roi);

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) override;
  void zoom(float scale) override;

  // rotate and orbit funstions are null operation for FlatCamera
  void rotate(const glm::vec2&, const glm::vec2&) override final {}
  void orbit(const glm::vec3&, float) override final {};
  void orbitHorizontal(float) override final {};
  void orbitVertical(float) override final {};

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(FlatCamera, Camera);
};

NAMESPACE_END(Vulk)