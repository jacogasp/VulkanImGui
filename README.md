# Vulkan ImGui

This library is provides a simple interface to quickly build a C++ application
with [Dear ImGui](https://github.com/ocornut/imgui),
using the [GLFW](https://github.com/glfw/glfw) library with [Vulkan](https://www.vulkan.org) backend.

## Requirements

*Vulkan SDK* is required, please install it from [here](https://vulkan.lunarg.com).

## Installation

In your repository, add this project as submodule

```shell
git submodule add git@github.com:jacogasp/VulkanImGui.git
```

then recursively update the inner submodules with

```shell
git submodule update --init --recursive
```

## CMake Setup

In order to properly compile everything, create a `CMakeLists.txt` file similarly to

```cmake
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
project(VulkanImGuiExample)

## Vulkan
find_package(Vulkan REQUIRED)

## VulkanImGui, ImGui and GLFW
add_subdirectory(VulkanImGui EXCLUDE_FROM_ALL)

SET(LIBRARIES VulkanImGui glfw imgui Vulkan::Vulkan)

## Main Executable
add_executable(Main main.cpp)
target_link_libraries(Main PRIVATE ${LIBRARIES})
```

## Implement your application

Here an example to create your application.
First create a class `MyApp` that implements the `void Update()` function. Here is where you build your ImGui interface.

```c++
#include "VulkanImGui/ImGuiApp.hpp"
#include "imgui.h"

class MyApp {
public:
    // Called each frame
    void Update() {
    static int counter = 0;
    ImGui::Begin("Hello, world!");
    if (ImGui::Button("Button"))
      counter++;
    ImGui::Text("counter = %d", counter);
    ImGui::Text(
        "Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate,
        ImGui::GetIO().Framerate
    );
    ImGui::End();
  }
};
```

In [`main.cpp`](main.cpp), then simply write

```c++
int main() {
  KCE::AppSettings appSettings{};
  appSettings.title = "My Vulkan+ImGui App";
  
  KCE::App<MyApp> app{appSettings};
  app.Run();
  
  return 0;
}
```

In [`main.cpp`](main.cpp) you find the complete example.
