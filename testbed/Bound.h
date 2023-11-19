#pragma once

#include <Vulk/helpers_debug.h>

#include <glm/glm.hpp>

#include <array>

template <typename T>
class Bound {
 public:
  using position_type = T;

  static constexpr T max() { return T{std::numeric_limits<typename T::value_type>::max()}; }
  static constexpr T min() { return T{std::numeric_limits<typename T::value_type>::min()}; }

  struct Face {
    std::array<T, 4> v;

    Face() = default;
    Face(const T& a, const T& b, const T& c, const T& d) : v{a, b, c, d} {}

    [[nodiscard]] T normal() const { return glm::normalize(glm::cross(v[2] - v[1], v[0] - v[1])); }
    // center of the face
    [[nodiscard]] T center() const { return (v[0] + v[1] + v[2] + v[3]) / 4.0f; }
    // up vector of the face
    [[nodiscard]] T up() const { return glm::normalize(v[0] - v[1]); }
  };

 public:
  Bound() = default;
  Bound(const T& lower, const T& upper) : _lower{lower}, _upper{upper} {}

  static Bound infinite() { return Bound{min(), max()}; }
  static Bound null() { return Bound{max(), min()}; }
  static Bound unit() { return Bound{T{-1}, T{1}}; }

  [[nodiscard]] const T& lower() const { return _lower; }
  [[nodiscard]] const T& upper() const { return _upper; }

  void setLower(const T& lower) { set(lower, _upper); }
  void setUpper(const T& upper) { set(_lower, upper); }

  void set(const T& lower, const T& upper) {
    MI_ASSERT(glm::all(glm::lessThan(lower, upper)));
    _lower = lower;
    _upper = upper;
  }

  Bound operator+(const T& point) const {
    return Bound{glm::min(point, _lower), glm::max(point, _upper)};
  }
  void operator+=(const T& point) {
    _lower = glm::min(point, _lower);
    _upper = glm::max(point, _upper);
  }

  Bound operator&(const Bound& rhs) const {
    return Bound{glm::max(_lower, rhs._lower), glm::min(_upper, rhs._upper)};
  }
  void operator&=(const Bound& rhs) {
    _lower = glm::max(_lower, rhs._lower);
    _upper = glm::min(_upper, rhs._upper);
  }

  [[nodiscard]] T center() const { return (_lower + _upper) / 2.0f; }
  [[nodiscard]] T extent() const { return _upper - _lower; };

  [[nodiscard]] Face front() const {
    return Face{_lower,
                T{_lower.x, _upper.y, _lower.z},
                T{_upper.x, _upper.y, _lower.z},
                T{_upper.x, _lower.y, _lower.z}};
  }

  void scale(float multiplier) {
    MI_ASSERT(multiplier > 0.0F);
    auto center = this->center();
    auto extent = this->extent();
    auto offset = extent * multiplier * 0.5F;
    _lower      = center - offset;
    _upper      = center + offset;
  }

  // aspect = width / height
  void fit(float aspect) {
    auto center = this->center();
    auto extent = this->extent();
    auto offset = extent * 0.5F;
    if (aspect > 1.0F) {
      offset.x *= aspect;
    } else {
      offset.y /= aspect;
    }
    _lower = center - offset;
    _upper = center + offset;
  }

  void move(const T& offset) {
    _lower += offset;
    _upper += offset;
  }

 protected:
  T _lower{};
  T _upper{};
};
