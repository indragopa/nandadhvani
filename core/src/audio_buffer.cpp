#include "nandadhvani/core/audio_buffer.hpp"

#include <algorithm>
#include <limits>

namespace nd {
AudioBuffer::AudioBuffer(std::uint32_t rate, std::uint16_t channel_count)
    : sample_rate(rate), channels(channel_count) {
    if (rate == 0 || channel_count == 0 || channel_count > 32)
        throw std::invalid_argument("invalid audio format");
}
std::size_t AudioBuffer::frames() const noexcept {
    return channels ? samples.size() / channels : 0;
}
float& AudioBuffer::at(std::size_t frame, std::size_t channel) {
    if (channel >= channels || frame >= frames())
        throw std::out_of_range("audio sample");
    return samples[frame * channels + channel];
}
const float& AudioBuffer::at(std::size_t frame, std::size_t channel) const {
    if (channel >= channels || frame >= frames())
        throw std::out_of_range("audio sample");
    return samples[frame * channels + channel];
}
void AudioBuffer::append_frames(const float* data, std::size_t count) {
    if (count == 0)
        return;
    if (!data)
        throw std::invalid_argument("null samples");
    if (count > (std::numeric_limits<std::size_t>::max() - samples.size()) / channels)
        throw std::length_error("audio too large");
    samples.insert(samples.end(), data, data + count * channels);
}
AudioBuffer AudioBuffer::slice(std::size_t first, std::size_t count) const {
    if (first > frames() || count > frames() - first)
        throw std::out_of_range("audio slice");
    AudioBuffer result(sample_rate, channels);
    const auto begin = samples.begin() + static_cast<std::ptrdiff_t>(first * channels);
    result.samples.assign(begin, begin + static_cast<std::ptrdiff_t>(count * channels));
    return result;
}
}  // namespace nd
