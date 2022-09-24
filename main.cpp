//
// Created by Jacopo Gasparetto on 19/09/22.
//
#include "ImGuiApp.hpp"
#include "imgui.h"
#include <array>
#include <cmath>

template <size_t size>
using plot_data = std::array<float, size>;

template <size_t size>
constexpr plot_data<size> makeBarPlotData() {
  plot_data<size> a;
  for (size_t i = 0; i < size; i++)
    a.at(i) = float(i) * 10.0f;
  return a;
}

template <size_t size>
plot_data<size> makeLinePlotData() {
  plot_data<size> a;
  for (size_t i = 0; i < size; ++i)
    a.at(i) = std::sin((float)i);
  return a;
}

class MyApp {
private:
  constexpr static plot_data<10> m_barPlotData{makeBarPlotData<10>()};
  plot_data<100> m_linePlotData;

public:
  MyApp() : m_linePlotData{makeLinePlotData<100>()} {}

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
    ImPlot::BeginPlot("A plot");
    ImPlot::PlotBars("Bar plot", m_barPlotData.data(), m_barPlotData.size());
    ImPlot::EndPlot();
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