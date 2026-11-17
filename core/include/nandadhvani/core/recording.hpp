#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "audio_buffer.hpp"
namespace nd {
// Worker-thread streamed float WAV writer. Audio callbacks should enqueue frames
// into SpscRingBuffer; never call this disk writer from the real-time thread.
class CrashSafeWavWriter {
   public:
    CrashSafeWavWriter(std::filesystem::path temp_path, std::uint32_t sample_rate, std::uint16_t channels,
                       std::uint64_t header_update_frames = 48000);
    ~CrashSafeWavWriter();
    CrashSafeWavWriter(const CrashSafeWavWriter&) = delete;
    CrashSafeWavWriter& operator=(const CrashSafeWavWriter&) = delete;
    void append(const float* interleaved, std::size_t frames);
    void flush();
    void finish();
    [[nodiscard]] std::uint64_t frames_written() const noexcept {
        return frames_;
    }
    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

   private:
    std::filesystem::path path_;
    std::fstream file_;
    std::uint32_t sample_rate_;
    std::uint16_t channels_;
    std::uint64_t frames_{};
    std::uint64_t last_header_frame_{};
    std::uint64_t interval_;
    bool finished_{};
    void update_header();
};
}  // namespace nd
