#include "nandadhvani/core/dsp.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace nd {
namespace {
constexpr float eps = 1e-20F;
float db(float x) {
    return 20.0F * std::log10(std::max(x, eps));
}
}  // namespace
Levels measure(const AudioBuffer& a) {
    Levels l;
    if (a.samples.empty())
        return l;
    double sum = 0;
    for (float x : a.samples) {
        l.peak = std::max(l.peak, std::abs(x));
        sum += static_cast<double>(x) * x;
    }
    l.rms = static_cast<float>(std::sqrt(sum / a.samples.size()));
    l.peak_db = db(l.peak);
    l.rms_db = db(l.rms);
    l.clipped = l.peak >= 1.0F;
    return l;
}
void normalize_peak(AudioBuffer& a, float target_db) {
    auto l = measure(a);
    if (l.peak < eps)
        return;
    const float gain = std::pow(10.0F, target_db / 20.0F) / l.peak;
    for (float& x : a.samples)
        x *= gain;
}
void fade(AudioBuffer& a, bool in, std::uint64_t count, bool power) {
    const auto n = std::min<std::uint64_t>(count, a.frames());
    if (!n)
        return;
    for (std::uint64_t i = 0; i < n; ++i) {
        double t = n == 1 ? 1.0 : static_cast<double>(i) / (n - 1);
        if (!in)
            t = 1.0 - t;
        const float g = static_cast<float>(power ? std::sin(t * 1.5707963267948966) : t);
        auto frame = in ? i : a.frames() - n + i;
        for (std::size_t c = 0; c < a.channels; ++c)
            a.samples[static_cast<std::size_t>(frame * a.channels + c)] *= g;
    }
}
float Biquad::process(float x, std::size_t ch) {
    if (ch >= 32)
        throw std::out_of_range("channel");
    double y = b0 * x + z1[ch];
    z1[ch] = b1 * x - a1 * y + z2[ch];
    z2[ch] = b2 * x - a2 * y;
    return static_cast<float>(y);
}
Biquad make_notch(double sr, double f, double q) {
    if (sr <= 0 || f <= 0 || f >= sr / 2 || q <= 0)
        throw std::invalid_argument("invalid notch parameters");
    double w = 2 * 3.14159265358979323846 * f / sr, alpha = std::sin(w) / (2 * q), a0 = 1 + alpha;
    Biquad b;
    b.b0 = 1 / a0;
    b.b1 = -2 * std::cos(w) / a0;
    b.b2 = 1 / a0;
    b.a1 = b.b1;
    b.a2 = (1 - alpha) / a0;
    return b;
}
Biquad make_lowpass(double sr, double f, double q) {
    if (sr <= 0 || f <= 0 || f >= sr / 2 || q <= 0)
        throw std::invalid_argument("invalid lowpass parameters");
    double w = 2 * 3.14159265358979323846 * f / sr, alpha = std::sin(w) / (2 * q), c = std::cos(w),
           a0 = 1 + alpha;
    Biquad b;
    b.b0 = (1 - c) / (2 * a0);
    b.b1 = (1 - c) / a0;
    b.b2 = b.b0;
    b.a1 = -2 * c / a0;
    b.a2 = (1 - alpha) / a0;
    return b;
}
void apply_biquad(AudioBuffer& a, Biquad& b) {
    for (std::size_t i = 0; i < a.frames(); ++i)
        for (std::size_t c = 0; c < a.channels; ++c)
            a.samples[i * a.channels + c] = b.process(a.samples[i * a.channels + c], c);
}
void compress(AudioBuffer& a, float threshold, float ratio, float attack, float release, float makeup) {
    if (ratio < 1 || attack <= 0 || release <= 0)
        throw std::invalid_argument("invalid compressor parameters");
    const float th = std::pow(10.0F, threshold / 20), make = std::pow(10.0F, makeup / 20);
    const double sr = a.sample_rate;
    double env = 0;
    const double at = std::exp(-1.0 / (sr * attack / 1000.0)), rel = std::exp(-1.0 / (sr * release / 1000.0));
    for (std::size_t i = 0; i < a.frames(); ++i) {
        float peak = 0;
        for (std::size_t c = 0; c < a.channels; ++c)
            peak = std::max(peak, std::abs(a.samples[i * a.channels + c]));
        double coeff = peak > env ? at : rel;
        env = coeff * env + (1 - coeff) * peak;
        float gain = 1;
        if (env > th) {
            float out = th * std::pow(static_cast<float>(env / th), 1.0F / ratio);
            gain = out / static_cast<float>(env);
        }
        for (std::size_t c = 0; c < a.channels; ++c)
            a.samples[i * a.channels + c] *= gain * make;
    }
}
void remove_hum(AudioBuffer& a, double f, double q, unsigned harmonics) {
    for (unsigned h = 1; h <= harmonics; ++h) {
        double hz = f * h;
        if (hz >= a.sample_rate * 0.49)
            break;
        auto b = make_notch(a.sample_rate, hz, q);
        apply_biquad(a, b);
    }
}
void spectral_gate(AudioBuffer& a, float threshold, float reduction) {
    const float th = std::pow(10.0F, threshold / 20),
                red = std::pow(10.0F, reduction / 20);  // smooth sample-domain expander fallback
    float env = 0;
    const float alpha = static_cast<float>(std::exp(-1.0 / (a.sample_rate * 0.012)));
    for (float& x : a.samples) {
        env = alpha * env + (1 - alpha) * std::abs(x);
        if (env < th)
            x *= red;
    }
}
void crossfade(AudioBuffer& l, AudioBuffer& r, std::size_t n) {
    if (l.channels != r.channels || l.sample_rate != r.sample_rate)
        throw std::invalid_argument("crossfade format mismatch");
    n = std::min({n, l.frames(), r.frames()});
    for (std::size_t i = 0; i < n; ++i) {
        double t = n == 1 ? 0.5 : static_cast<double>(i) / (n - 1);
        float a = static_cast<float>(std::cos(t * 1.5707963267948966)),
              b = static_cast<float>(std::sin(t * 1.5707963267948966));
        for (std::size_t c = 0; c < l.channels; ++c) {
            l.samples[(l.frames() - n + i) * l.channels + c] *= a;
            r.samples[i * l.channels + c] *= b;
        }
    }
}
}  // namespace nd
