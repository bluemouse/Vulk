
#include "Testbed.h"

#include <cstdlib>
#include <iostream>

#include <cxxopts.hpp>

int main(int argc, char** argv) {
  cxxopts::Options options("Testbed", "Testbed for Vulk");

#if defined(_NDEBUG)
  // None
  #define DEFAULT_VALIDATION "0"
#else
  // Warning
  #define DEFAULT_VALIDATION "2"
#endif

      options.add_options()(
          "v,validation",
          "Set Vulkan validation level (0: none, 1: error, 2: warning, 3: info, 4: verbose)",
          cxxopts::value<int>()->default_value(DEFAULT_VALIDATION))(
          "r,reflect",
          "Print SPIRV-Reflect information",
          cxxopts::value<bool>()->default_value("false"))
          ("h,help", "Print usage");

  auto result = options.parse(argc, argv);

  if (result.count("help"))
  {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  Testbed::setValidationLevel(static_cast<Testbed::ValidationLevel>(result["validation"].as<int>()));
  Testbed::setPrintReflect(result["reflect"].as<bool>());

  Testbed testbed;

  constexpr int width = 800;
  constexpr int height = 800;
  testbed.init(width, height);
  try {
    testbed.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
