#include "App.h"

void App::init(Vulk::DeviceContext::shared_ptr deviceContext, const Params&) {
  _deviceContext = deviceContext.get();
}

App::Registry& App::registry() {
  static Registry registry;
  return registry;
}

#include "ModelViewer.h"
#include "ImageViewer.h"
#include "ParticlesViewer.h"
App::Registry::Registry() {
  add<ModelViewer>();
  add<ImageViewer>();
  add<ParticlesViewer>();
}

App* App::Registry::get(const std::string& id) const {
  try {
    return at(id);
  } catch (const std::out_of_range&) {
    return nullptr;
  }
}

void App::Registry::print(std::ostream& os) const {
  for (const auto& pair : *this) {
    os << pair.first << ": " << pair.second->description() << std::endl;
  }
}
