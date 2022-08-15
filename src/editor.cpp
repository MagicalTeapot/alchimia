#include "editor.hpp"

#include <cereal/archives/binary.hpp>
#include <imgui.h>

#include <fstream>

namespace sand {

auto display_ui(editor& editor, tile& tile, const timer& timer) -> void
{
    ImGui::ShowDemoWindow(&editor.show_demo);

    if (ImGui::Begin("Editor")) {
        for (std::size_t i = 0; i != editor.pixel_makers.size(); ++i) {
            if (ImGui::Selectable(editor.pixel_makers[i].first.c_str(), editor.current == i)) {
                editor.current = i;
            }
        }
        ImGui::SliderFloat("Brush size", &editor.brush_size, 0, 50);
        if (ImGui::Button("Clear")) {
            tile.fill(sand::pixel::air());
        }

        ImGui::Text("FPS: %d", timer.frame_rate());
        ImGui::Text("Awake chunks: %d", tile.num_awake_chunks());
        ImGui::Checkbox("Show chunks", &editor.show_chunks);

        if (ImGui::Button("Save")) {
            auto file = std::ofstream{"save.bin", std::ios::binary};
            auto archive = cereal::BinaryOutputArchive{file};
            archive(tile);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            auto file = std::ifstream{"save.bin", std::ios::binary};
            auto archive = cereal::BinaryInputArchive{file};
            archive(tile);
            tile.wake_all_chunks();
        }
        if (ImGui::RadioButton("Spray", editor.brush_type == 0)) {
            editor.brush_type = 0;
        }
        if (ImGui::RadioButton("Square", editor.brush_type == 1)) {
            editor.brush_type = 1;
        }
        ImGui::Text("Brush: %d", editor.brush_type);
        ImGui::SliderInt2("Top Left:", &editor.top_left.x, -100, 100);
        ImGui::SliderInt("Scale:", &editor.zoom, 100, 1000);
    }
    ImGui::End();
}
    
}