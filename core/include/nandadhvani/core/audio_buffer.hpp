#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace nd {
struct AudioBuffer {
    std::uint32_t sample_rate{48000};
    std::uint16_t channels{1};
    std::vector<float> samples;

    AudioBuffer() = default;
    AudioBuffer(std::uint32_t rate, std::uint16_t channel_count);
    [[nodiscard]] std::size_t frames() const noexcept;
    [[nodiscard]] bool empty() const noexcept {
        return samples.empty();
    }
    float& at(std::size_t frame, std::size_t channel);
    const float& at(std::size_t frame, std::size_t channel) const;
    void append_frames(const float* interleaved, std::size_t frame_count);
    [[nodiscard]] AudioBuffer slice(std::size_t first_frame, std::size_t frame_count) const;
};
}  // namespace nd
