//
// Created by Jacopo Gasparetto on 22/09/22.
//

#ifndef VulkanImGui_IMGUIAPP_HPP
#define VulkanImGui_IMGUIAPP_HPP

#include <iostream>
#include <utility>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "implot.h"
#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to
// maximize ease of testing and compatibility with old VS compilers. To link
// with VS2010-era libraries, VS2015+ requires linking with
// legacy_stdio_definitions.lib, which we do using this pragma. Your own project
// should not be affected, as you are likely to link with a newer binary of GLFW
// that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

namespace KCE {

static VkAllocationCallbacks *g_Allocator     = nullptr;
static VkInstance g_Instance                  = nullptr;
static VkPhysicalDevice g_PhysicalDevice      = nullptr;
static VkDevice g_Device                      = nullptr;
static uint32_t g_QueueFamily                 = (uint32_t)-1;
static VkQueue g_Queue                        = nullptr;
static VkDebugReportCallbackEXT g_DebugReport = nullptr;
static VkPipelineCache g_PipelineCache        = nullptr;
static VkDescriptorPool g_DescriptorPool      = nullptr;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int g_MinImageCount     = 2;
static bool g_SwapChainRebuild = false;

static void check_vk_result(VkResult err) {
  if (err == 0)
    return;
  std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
  if (err < 0)
    std::exit(err);
}

static void SetupVulkan(std::vector<const char *> extensions) {
  VkResult result;
  // Create Vulkan Instance
  {
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.enabledExtensionCount   = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    result                             = vkCreateInstance(&createInfo, g_Allocator, &g_Instance);
    check_vk_result(result);
    IM_UNUSED(g_DebugReport);
  }
  // Select GPU
  {
    uint32_t gpuCount;
    result = vkEnumeratePhysicalDevices(g_Instance, &gpuCount, nullptr);
    check_vk_result(result);
    IM_ASSERT(gpuCount > 0);

    std::vector<VkPhysicalDevice> gpus;
    gpus.resize(gpuCount);
    result = vkEnumeratePhysicalDevices(g_Instance, &gpuCount, gpus.data());
    check_vk_result(result);

    // Find discrete GPU if present, otherwise use the first one available.
    g_PhysicalDevice = gpus.at(0);
    VkPhysicalDeviceProperties properties;
    for (auto &gpu : gpus) {
      vkGetPhysicalDeviceProperties(gpu, &properties);
      if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        g_PhysicalDevice = gpu;
        break;
      }
    }
    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &properties);
    std::cout << "Selected GPU: " << properties.deviceName << std::endl;
  }

  // Select graphics queue family
  {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues;
    queues.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues.data());
    for (uint32_t i = 0; i < count; ++i) {
      if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        g_QueueFamily = i;
        break;
      }
    }
    IM_ASSERT(g_QueueFamily != (uint32_t)-1);
  }

  // Create Logical Device (with 1 queue)
  {
    const char *deviceExtensions[]{"VK_KHR_swapchain"};
    const float queuePriority[]{1.0f};
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = g_QueueFamily;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = queuePriority;
    VkDeviceQueueCreateInfo queueCreateInfos[]{{queueInfo}};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pQueueCreateInfos       = queueCreateInfos;
    createInfo.enabledExtensionCount   = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    result                             = vkCreateDevice(g_PhysicalDevice, &createInfo, g_Allocator, &g_Device);
    check_vk_result(result);
    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
  }

  // Create Descriptor Pool
  {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(poolSizes);
    pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(poolSizes);
    pool_info.pPoolSizes                 = poolSizes;
    result                               = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
    check_vk_result(result);
  }
}

static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height) {
  wd->Surface = surface;
  // Check for WSI support
  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
  if (res != VK_TRUE) {
    std::cerr << "Error no WSI support on physical device 0\n";
    std::exit(-1);
  }
  // Select Surface Format
  const VkFormat requestSurfaceImageFormat[]{
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_FORMAT_B8G8R8A8_UNORM};
  const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      g_PhysicalDevice,
      wd->Surface,
      requestSurfaceImageFormat,
      (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
      requestSurfaceColorSpace
  );

  // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_FRATE
  VkPresentModeKHR present_modes[] = {
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_FIFO_KHR};
#else
  VkPresentModeKHR presetModes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
      g_PhysicalDevice,
      wd->Surface,
      &presetModes[0],
      IM_ARRAYSIZE(presetModes)
  );
  // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

  // Create SwapChain, RenderPass, Frame buffer, etc.
  IM_ASSERT(g_MinImageCount >= 2);
  ImGui_ImplVulkanH_CreateOrResizeWindow(
      g_Instance,
      g_PhysicalDevice,
      g_Device,
      wd,
      g_QueueFamily,
      g_Allocator,
      width,
      height,
      g_MinImageCount
  );
}

static void CleanupVulkan() {
  vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // Remove the debug report callback
  auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT
  )vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
  vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

  vkDestroyDevice(g_Device, g_Allocator);
}

static void CleanupVulkanWindow() {
  ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data) {
  VkResult result;
  VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;

  result = vkAcquireNextImageKHR(
      g_Device,
      wd->Swapchain,
      UINT64_MAX,
      image_acquired_semaphore,
      VK_NULL_HANDLE,
      &wd->FrameIndex
  );
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    g_SwapChainRebuild = true;
    return;
  }
  check_vk_result(result);

  ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
  {
    result = vkWaitForFences(
        g_Device,
        1,
        &fd->Fence,
        VK_TRUE,
        UINT64_MAX
    ); // wait indefinitely instead of periodically checking
    check_vk_result(result);

    result = vkResetFences(g_Device, 1, &fd->Fence);
    check_vk_result(result);
  }
  {
    result = vkResetCommandPool(g_Device, fd->CommandPool, 0);
    check_vk_result(result);
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    check_vk_result(result);
  }
  {
    VkRenderPassBeginInfo info    = {};
    info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass               = wd->RenderPass;
    info.framebuffer              = fd->Framebuffer;
    info.renderArea.extent.width  = wd->Width;
    info.renderArea.extent.height = wd->Height;
    info.clearValueCount          = 1;
    info.pClearValues             = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info               = {};
    info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount         = 1;
    info.pWaitSemaphores            = &image_acquired_semaphore;
    info.pWaitDstStageMask          = &wait_stage;
    info.commandBufferCount         = 1;
    info.pCommandBuffers            = &fd->CommandBuffer;
    info.signalSemaphoreCount       = 1;
    info.pSignalSemaphores          = &render_complete_semaphore;

    result = vkEndCommandBuffer(fd->CommandBuffer);
    check_vk_result(result);
    result = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
    check_vk_result(result);
  }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd) {
  if (g_SwapChainRebuild)
    return;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;

  VkPresentInfoKHR info   = {};
  info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores    = &render_complete_semaphore;
  info.swapchainCount     = 1;
  info.pSwapchains        = &wd->Swapchain;
  info.pImageIndices      = &wd->FrameIndex;

  VkResult result = vkQueuePresentKHR(g_Queue, &info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    g_SwapChainRebuild = true;
    return;
  }
  check_vk_result(result);
  wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char *description) {
  std::cerr << "Glwf Error " << error << ": " << description << std::endl;
}

struct AppSettings {
  int width         = 1280;
  int height        = 720;
  std::string title = "Dear ImGui GLFW+Vulkan example";
  bool showDemo = false;
};

template <typename Derived>
class App : public Derived {
  AppSettings m_settings;
  ImGuiConfigFlags m_imGuiConfigFlags{0};
  ImGui_ImplVulkanH_Window *wd = nullptr;
  GLFWwindow *window           = nullptr;
  bool show_demo_window        = true;
  bool show_another_window     = false;
  ImVec4 clear_color           = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

public:
  explicit App(AppSettings appSettings = AppSettings{}) : m_settings{std::move(appSettings)} { Init(); }
  ~App() { Cleanup(); }
  void Update() { static_cast<Derived *>(this)->Update(); };
  void Run() {
    while (!glfwWindowShouldClose(window)) {
      // Poll and handle events (inputs, window resize, etc.)
      // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your
      // inputs.
      // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or
      // clear/overwrite your copy of the mouse data.
      // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or
      // clear/overwrite your copy of the keyboard data. Generally you may always pass all inputs to dear imgui, and
      // hide them from your application based on those two flags.
      glfwPollEvents();

      // Resize swap chain?
      if (g_SwapChainRebuild) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width > 0 && height > 0) {
          ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
          ImGui_ImplVulkanH_CreateOrResizeWindow(
              g_Instance,
              g_PhysicalDevice,
              g_Device,
              &g_MainWindowData,
              g_QueueFamily,
              g_Allocator,
              width,
              height,
              g_MinImageCount
          );
          g_MainWindowData.FrameIndex = 0;
          g_SwapChainRebuild          = false;
        }
      }

      // Start the Dear ImGui frame
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      Update();

      // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to
      // learn more about Dear ImGui!).
      if (m_settings.showDemo)
        ImGui::ShowDemoWindow(&m_settings.showDemo);

      // Rendering
      ImGui::Render();
      ImDrawData *main_draw_data   = ImGui::GetDrawData();
      const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
      wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
      wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
      wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
      wd->ClearValue.color.float32[3] = clear_color.w;
      if (!main_is_minimized)
        FrameRender(wd, main_draw_data);

      // Update and Render additional Platform Windows
      if (m_imGuiConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
      }

      // Present Main Platform Window
      if (!main_is_minimized)
        FramePresent(wd);
    }
  }

private:
  void Init() {
    // Setup GLFW window
    glfwSetErrorCallback(KCE::glfw_error_callback);
    if (!glfwInit())
      std::exit(1);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(m_settings.width, m_settings.height, m_settings.title.c_str(), nullptr, nullptr);

    // Setup Vulkan
    if (!glfwVulkanSupported()) {
      printf("GLFW: Vulkan Not Supported\n");
      std::exit(1);
    }
    uint32_t extensions_count   = 0;
    const char **extensions_ptr = glfwGetRequiredInstanceExtensions(&extensions_count);
    std::vector<const char *> extensions;
    for (uint32_t i = 0; i < extensions_count; i++) {
      extensions.push_back(extensions_ptr[i]);
    }
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(result);

    // Create Frame buffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;
    m_imGuiConfigFlags = io.ConfigFlags;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
    // ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      style.WindowRounding              = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = g_Instance;
    init_info.PhysicalDevice            = g_PhysicalDevice;
    init_info.Device                    = g_Device;
    init_info.QueueFamily               = g_QueueFamily;
    init_info.Queue                     = g_Queue;
    init_info.PipelineCache             = g_PipelineCache;
    init_info.DescriptorPool            = g_DescriptorPool;
    init_info.Subpass                   = 0;
    init_info.MinImageCount             = g_MinImageCount;
    init_info.ImageCount                = wd->ImageCount;
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator                 = g_Allocator;
    init_info.CheckVkResultFn           = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
    // ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application
    // (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double
    // backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
    // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

    // Upload Fonts
    {
      // Use any command queue
      VkCommandPool command_pool     = wd->Frames[wd->FrameIndex].CommandPool;
      VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

      result = vkResetCommandPool(g_Device, command_pool, 0);
      check_vk_result(result);
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      result = vkBeginCommandBuffer(command_buffer, &begin_info);
      check_vk_result(result);

      ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

      VkSubmitInfo end_info       = {};
      end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      end_info.commandBufferCount = 1;
      end_info.pCommandBuffers    = &command_buffer;
      result                      = vkEndCommandBuffer(command_buffer);
      check_vk_result(result);
      result = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
      check_vk_result(result);

      result = vkDeviceWaitIdle(g_Device);
      check_vk_result(result);
      ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
  }
  void Cleanup() {
    // Cleanup
    auto result = vkDeviceWaitIdle(g_Device);
    check_vk_result(result);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();

    CleanupVulkanWindow();
    CleanupVulkan();

    glfwDestroyWindow(window);
    glfwTerminate();
  }
};
} // namespace KCE

#endif // VulkanImGui_IMGUIAPP_HPP
