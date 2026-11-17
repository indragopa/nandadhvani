#pragma once
#include <vector>

#include "audio_buffer.hpp"
namespace nd {
struct Levels {
    float peak{};
    float rms{};
    float peak_db{-120.0F};
    float rms_db{-120.0F};
    bool clipped{};
};
Levels measure(const AudioBuffer& audio);
void normalize_peak(AudioBuffer& audio, float target_db = -1.0F);
void fade(AudioBuffer& audio, bool fade_in, std::uint64_t frames, bool equal_power = false);
struct Biquad {
    double b0{1}, b1{}, b2{}, a1{}, a2{};
    double z1[32]{}, z2[32]{};
    float process(float x, std::size_t channel = 0);
};
Biquad make_notch(double sample_rate, double frequency, double q);
Biquad make_lowpass(double sample_rate, double frequency, double q = 0.70710678);
void apply_biquad(AudioBuffer& audio, Biquad& filter);
void compress(AudioBuffer& audio, float threshold_db = -18, float ratio = 3, float attack_ms = 10,
              float release_ms = 100, float makeup_db = 0);
void remove_hum(AudioBuffer& audio, double frequency = 50.0, double q = 25.0, unsigned harmonics = 8);
void spectral_gate(AudioBuffer& audio, float threshold_db = -48.0F, float reduction_db = -18.0F);
void crossfade(AudioBuffer& left, AudioBuffer& right, std::size_t frames);
}  // namespace nd
