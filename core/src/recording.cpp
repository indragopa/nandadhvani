#include "nandadhvani/core/recording.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace nd {
namespace {
void put16(char* p, std::uint16_t v) {
    p[0] = static_cast<char>(v);
    p[1] = static_cast<char>(v >> 8);
}
void put32(char* p, std::uint32_t v) {
    for (int i = 0; i < 4; ++i)
        p[i] = static_cast<char>(v >> (i * 8));
}
}  // namespace
CrashSafeWavWriter::CrashSafeWavWriter(std::filesystem::path p, std::uint32_t sr, std::uint16_t ch,
                                       std::uint64_t interval)
    : path_(std::move(p)), sample_rate_(sr), channels_(ch), interval_(std::max<std::uint64_t>(1, interval)) {
    if (!sr || !ch || ch > 32)
        throw std::invalid_argument("invalid recording format");
    file_.open(path_, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    if (!file_)
        throw std::runtime_error("cannot create recording temp file");
    char h[44]{};
    std::memcpy(h, "RIFF", 4);
    std::memcpy(h + 8, "WAVEfmt ", 8);
    put32(h + 16, 16);
    put16(h + 20, 3);
    put16(h + 22, ch);
    put32(h + 24, sr);
    put32(h + 28, sr * ch * 4);
    put16(h + 32, static_cast<std::uint16_t>(ch * 4));
    put16(h + 34, 32);
    std::memcpy(h + 36, "data", 4);
    file_.write(h, 44);
    if (!file_)
        throw std::runtime_error("cannot initialize recording");
}
CrashSafeWavWriter::~CrashSafeWavWriter() {
    if (file_.is_open()) {
        try {
            flush();
        } catch (...) {
        }
    }
}
void CrashSafeWavWriter::append(const float* data, std::size_t n) {
    if (finished_)
        throw std::logic_error("recording already finalized");
    if (n == 0)
        return;
    if (!data)
        throw std::invalid_argument("null recording samples");
    if (n > std::numeric_limits<std::uint64_t>::max() - frames_)
        throw std::length_error("recording frame count overflow");
    const std::uint64_t next = frames_ + n;
    if (next > (0xffffffffULL - 36) / (static_cast<std::uint64_t>(channels_) * 4))
        throw std::length_error(
            "recording exceeded RIFF/WAV 4 GB limit; RF64 recording is not yet available");
    const auto byte_count = n * channels_ * sizeof(float);
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        throw std::length_error("recording block is too large");
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(byte_count));
    if (!file_)
        throw std::runtime_error("recording disk write failed");
    frames_ = next;
    if (frames_ - last_header_frame_ >= interval_)
        update_header();
}
void CrashSafeWavWriter::update_header() {
    const auto bytes = static_cast<std::uint32_t>(frames_ * channels_ * 4);
    file_.flush();
    file_.seekp(4);
    char sz[4];
    put32(sz, bytes + 36);
    file_.write(sz, 4);
    file_.seekp(40);
    put32(sz, bytes);
    file_.write(sz, 4);
    file_.flush();
    if (!file_)
        throw std::runtime_error("recording header update failed");
    file_.seekp(0, std::ios::end);
    last_header_frame_ = frames_;
}
void CrashSafeWavWriter::flush() {
    if (!file_.is_open())
        return;
    update_header();
}
void CrashSafeWavWriter::finish() {
    if (finished_)
        return;
    flush();
    file_.close();
    finished_ = true;
}
}  // namespace nd
