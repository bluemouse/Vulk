#pragma once

#include <Vulk/engine/DeviceContext.h>
#include <Vulk/engine/FrameContext.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

#include <filesystem>

class App {
 public:
  constexpr static std::string PARAM_MODEL_FILE   = "model";
  constexpr static std::string PARAM_TEXTURE_FILE = "texture";

 public:
  class Param {
   public:
    virtual ~Param() = default;

    template <typename T>
    const T& value() const {
      return dynamic_cast<const Parameter<T>&>(*this).value;
    }
  };

  template <typename T>
  class Parameter : public Param {
   public:
    Parameter(const T& value) : value(value) {}

   private:
    T value;

    friend class Param;
  };

  struct Params : public std::map<std::string, const Param*> {
    ~Params() {
      for (auto& pair : *this) {
        delete pair.second;
      }
    }
    const Param* operator[](const std::string& name) const {
      try {
        return at(name);
      } catch (const std::out_of_range&) {
        return nullptr;
      }
    }

    template <typename T>
    void add(const std::string& name, const T& param) {
      insert({name, new Parameter<T>{param}});
    }
  };

 public:
  virtual ~App() = default;

  virtual void init(const Params& params)      = 0;
  virtual void render()                        = 0;
  virtual void cleanup()                       = 0;
  virtual void resize(uint width, uint height) = 0;

  virtual Vulk::Camera& camera() = 0;
};
