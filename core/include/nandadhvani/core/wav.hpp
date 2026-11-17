#pragma once
#include <filesystem>
#include <string>

#include "audio_buffer.hpp"
namespace nd {
enum class WavEncoding { Pcm16, Pcm24, Pcm32, Float32 };
struct WavInfo {
    std::uint32_t sample_rate{};
    std::uint16_t channels{};
    std::uint16_t bits_per_sample{};
    bool floating_point{};
    std::uint64_t frames{};
};
void write_wav(const std::filesystem::path& path, const AudioBuffer& audio,
               WavEncoding encoding = WavEncoding::Float32);
AudioBuffer read_wav(const std::filesystem::path& path);
WavInfo inspect_wav(const std::filesystem::path& path);
}  // namespace nd
