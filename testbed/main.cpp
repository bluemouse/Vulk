
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
      "app",
      "Select the app to run",
      cxxopts::value<std::string>()
    )
    (
      "list-apps",
      "List all available apps"
    )
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
      "v, validation-level",
      "Set Vulkan validation level (0: none, 1: error, 2: warning, 3: info, 4: verbose)",
      cxxopts::value<int>()->default_value(kDefaultValidation)
    )
    (
      "debug-utils",
      "Enable Vulkan debug-utils extension to enable object tagging nad queue labeling. Enabled by default if validation-level is not 0",
      cxxopts::value<bool>()->default_value("false")
    )
    (
      "spirv-reflect-info",
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

  if (options.count("list-apps")) {
    App::registry().print(std::cout);
    return EXIT_SUCCESS;
  }

  auto validationLevel = options["validation-level"].as<int>();  ;
  Testbed::setVulkanValidationLevel(static_cast<Testbed::ValidationLevel>(validationLevel));
  Testbed::setVulkanDebugUtilsExt(validationLevel ? true : options["debug-utils"].as<bool>());
  Testbed::setPrintSpirvReflect(options["spirv-reflect-info"].as<bool>());
  Testbed::setContinuousUpdate(options["continuous-update"].as<bool>());

  Testbed testbed;

  if (options.count("app")) {
    testbed.setApp(options["app"].as<std::string>());
  }
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
