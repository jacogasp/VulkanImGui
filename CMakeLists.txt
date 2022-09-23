cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
project(VulkanImGui)


## GLFW
set(GLFW_SOURCE_DIR libs/glfw)
add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
include_directories("${GLFW_SOURCE_DIR}/deps")

## Vulkan
find_package(Vulkan REQUIRED)
# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

## ImGui
set(IMGUI_SOURCE_DIR libs/imgui)
add_library(imgui STATIC EXCLUDE_FROM_ALL)
target_sources(
        imgui
        PRIVATE
        ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/imgui.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_include_directories(
        imgui
        PUBLIC ${IMGUI_SOURCE_DIR}
        PUBLIC ${IMGUI_SOURCE_DIR}/backends
)
target_link_libraries(imgui PRIVATE glfw Vulkan::Vulkan)

## VulkanImGui (this library)
add_library(VulkanImGui STATIC ImGuiApp.cpp)
target_link_libraries(VulkanImGui PRIVATE imgui glfw Vulkan::Vulkan)

### Executable example
add_executable(
        VulkanImGuiExample
        main.cpp
)
target_link_libraries(VulkanImGuiExample PRIVATE VulkanImGui imgui Vulkan::Vulkan glfw)