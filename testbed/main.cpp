
#include "Testbed.h"

#include <cstdlib>
#include <iostream>

int main() {
  Testbed testbed;

  constexpr int width = 800;
  constexpr int height = 600;
  testbed.init(width, height);
  try {
    testbed.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
