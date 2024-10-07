## Dependencies
- fmt: https://fmt.dev/ (`sudo apt install libfmt-dev`)

### Build Steps
1. cmake -S . -B build -G "Ninja" -D CMAKE_BUILD_TYPE=debug|release
2. cmake --build build -j 16

### Run Testbed
- build/bin/Testbed --help
- build/bin/Testbed
- build/bin/Testbed --model path_to_your_obj_file --texture path_to_your_texture_file



### References
1. https://registry.khronos.org/vulkan/specs/1.3/html/index.html
2. https://docs.vulkan.org/spec/latest/index.html
3. https://www.khronos.org/files/vulkan11-reference-guide.pdf
4. https://github.com/David-DiGioia/vulkan-diagrams
5. https://gpuopen.com/learn/understanding-vulkan-objects/