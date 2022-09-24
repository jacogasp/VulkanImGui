//
// Created by Jacopo Gasparetto on 19/09/22.
//
#include "ImGuiApp.hpp"
#include "imgui.h"
#include <array>
#include <cmath>

template <size_t size>
using plot_array = std::array<float, size>;

template<size_t Size>
struct PlotData {
  plot_array<Size> x;
  plot_array<Size> y;
  size_t size = Size;
};

template <size_t size>
constexpr plot_array<size> makeBarPlotData() {
  plot_array<size> a;
  for (size_t i = 0; i < size; i++)
    a.at(i) = float(i) * 10.0f;
  return a;
}

template <size_t size>
PlotData<size> makeLinePlotData(float a, float b) {
  plot_array<size> x;
  plot_array<size> y;
  for (size_t i = 0; i < size; ++i) {
    auto t = float(i) / size * (b - a);
    x.at(i) = t;
    y.at(i) = std::sin(t / M_PI_2) * 100.0f;
  }
  return {x, y};
}

class MyApp {
private:
  constexpr static size_t n_points = 1000;
  constexpr static plot_array<10> m_barPlotData{makeBarPlotData<10>()};
  PlotData<n_points> m_linePlotData;

public:
  MyApp() : m_linePlotData{makeLinePlotData<n_points>(0.0f, 10.0f)} {}

  void Update() {
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
    ImPlot::PlotBars("Bar Plot", m_barPlotData.data(), m_barPlotData.size());
    ImPlot::PlotLine("Line Plot", m_linePlotData.x.data(), m_linePlotData.y.data(),  m_linePlotData.size);
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