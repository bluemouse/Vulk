
#include "Testbed.h"

#include <cstdlib>
#include <iostream>

#include <cxxopts.hpp>

int main(int argc, char** argv) {
  cxxopts::Options supportedOptions("Testbed", "Testbed for Vulk");

#if defined(_NDEBUG)
  // None
  constexpr const char* kDefaultValidation = "0";
#else
  // Warning
  constexpr const char* kDefaultValidation = "2";
#endif
  // clang-format off
  supportedOptions.add_options()
    (
      "m,model",
      "Set the input model file (.obj file only)",
      cxxopts::value<std::string>()
    )
    (
      "t,texture",
      "Set the input texture file (.jpg/.png file only)",
      cxxopts::value<std::string>()
    )
    (
      "v,validation-level",
      "Set Vulkan validation level (0: none, 1: error, 2: warning, 3: info, 4: verbose)",
      cxxopts::value<int>()->default_value(kDefaultValidation)
    )
    (
      "r,reflect-info",
      "Print SPIRV-Reflect information",
      cxxopts::value<bool>()->default_value("false")
    )
    (
      "c,continuous-update",
      "Continuous rendering update even when there is no UI events",
      cxxopts::value<bool>()->default_value("false")
    )
    (
      "h,help",
      "Print usage"
    );
  // clang-format on

  auto options = supportedOptions.parse(argc, argv);

  if (options.count("help")) {
    std::cout << supportedOptions.help() << std::endl;
    return EXIT_SUCCESS;
  }

  Testbed::setValidationLevel(
      static_cast<Testbed::ValidationLevel>(options["validation-level"].as<int>()));
  Testbed::setPrintReflect(options["reflect-info"].as<bool>());
  Testbed::setContinuousUpdate(options["continuous-update"].as<bool>());

  Testbed testbed;

  if (options.count("model")) {
    testbed.setModelFile(options["model"].as<std::string>());
  }
  if (options.count("texture")) {
    testbed.setTextureFile(options["texture"].as<std::string>());
  }

  constexpr int width  = 960;
  constexpr int height = 540;
  testbed.init(width, height);
  try {
    testbed.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
