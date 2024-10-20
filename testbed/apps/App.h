#pragma once

#include <Vulk/engine/DeviceContext.h>
#include <Vulk/engine/FrameContext.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

#include <map>
#include <filesystem>

class App {
 public:
  constexpr static std::string PARAM_MODEL_FILE   = "model";
  constexpr static std::string PARAM_TEXTURE_FILE = "texture";

  class Params;

 public:
  App(const std::string& id, const std::string& description) : _id{id}, _description{description} {}
  virtual ~App() = default;

  virtual void init(Vulk::DeviceContext::shared_ptr deviceContext, const Params& params);
  virtual void render()                        = 0;
  virtual void cleanup()                       = 0;
  virtual void resize(uint width, uint height) = 0;

  [[nodiscard]] virtual Vulk::Camera& camera() = 0;

  [[nodiscard]] const std::string& id() const { return _id; }
  [[nodiscard]] const std::string& description() const { return _description; }

 protected:
  [[nodiscard]] Vulk::DeviceContext& deviceContext() { return *_deviceContext; }
  [[nodiscard]] const Vulk::DeviceContext& deviceContext() const { return *_deviceContext; }

 private:
  std::string _id;
  std::string _description;

  Vulk::DeviceContext* _deviceContext = nullptr;

 private:
  class _Param {
   public:
    _Param(const std::string& name) : _name{name} {}
    virtual ~_Param() = default;

    const std::string& name() const { return _name; }

    template <typename T>
    const T& value() const {
      return dynamic_cast<const _Parameter<T>&>(*this)._value;
    }

   private:
    std::string _name;
  };

  template <typename T>
  class _Parameter : public _Param {
   public:
    _Parameter(const std::string& name, const T& value) : _Param{name}, _value{value} {}

   private:
    T _value;

    friend class _Param;
  };

 public:
  class Params : public std::map<std::string, const _Param*> {
   public:
    ~Params() {
      for (auto& pair : *this) {
        delete pair.second;
      }
    }
    const _Param* operator[](const std::string& paramName) const {
      try {
        return at(paramName);
      } catch (const std::out_of_range&) {
        return nullptr;
      }
    }

    template <typename T>
    void add(const std::string& paramName, const T& paramValue) {
      insert({paramName, new _Parameter<T>{paramName, paramValue}});
    }
  };

  class Registry : private std::map<std::string, App*> {
   public:
    Registry();

    template <class App>
    void add() {
      App* app = new App;
      insert({app->id(), app});
    }
    [[nodiscard]] App* get(const std::string& id) const;

    void print(std::ostream& os) const;
  };
  static Registry& registry();
};
