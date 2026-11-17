#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "edl.hpp"
namespace nd {
struct TrackInfo {
    std::string name;
    std::string type{"Voice"};
    std::string color{"#48a9a6"};
    float gain{1.0F};
    float pan{};
    bool muted{};
    bool solo{};
    bool locked{};
    bool armed{};
    std::filesystem::path media;
};
struct ProjectData {
    std::string title{"Untitled"};
    std::string script_path;
    std::size_t script_position{};
    std::uint32_t sample_rate{48000};
    std::uint16_t channels{1};
    std::vector<TrackInfo> tracks;
    std::vector<Marker> markers;
};
void save_project(const std::filesystem::path& path, const ProjectData& project);
ProjectData load_project(const std::filesystem::path& path);
std::filesystem::path autosave_path(const std::filesystem::path& project_path);
}  // namespace nd
