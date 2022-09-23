//
// Created by Jacopo Gasparetto on 19/09/22.
//
#include "ImGuiApp.hpp"
#include "imgui.h"

class MyApp {
public:
  static void Update() {
    static float f     = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    if (ImGui::Button("Button"))
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);
    ImGui::Text(
        "Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate,
        ImGui::GetIO().Framerate
    );
    ImGui::End();
  }
};

int main() {
  KCE::AppSettings appSettings{};
  appSettings.title = "My Vulkan+ImGui App";
  KCE::App<MyApp> app{appSettings};
  app.Run();
  return 0;
}