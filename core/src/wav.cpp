#include "nandadhvani/core/wav.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace nd {
namespace {
void put16(std::ostream& o, std::uint16_t v) {
    o.put(static_cast<char>(v));
    o.put(static_cast<char>(v >> 8));
}
void put32(std::ostream& o, std::uint32_t v) {
    for (int i = 0; i < 4; ++i)
        o.put(static_cast<char>(v >> (i * 8)));
}
std::uint16_t get16(std::istream& i) {
    unsigned char b[2];
    i.read(reinterpret_cast<char*>(b), 2);
    if (!i)
        throw std::runtime_error("truncated WAV");
    return static_cast<std::uint16_t>(b[0] | (b[1] << 8));
}
std::uint32_t get32(std::istream& i) {
    unsigned char b[4];
    i.read(reinterpret_cast<char*>(b), 4);
    if (!i)
        throw std::runtime_error("truncated WAV");
    return static_cast<std::uint32_t>(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}
struct Header {
    WavInfo info;
    std::uint16_t format{};
    std::uint16_t block{};
    std::uint32_t data_bytes{};
    std::streampos data_pos{};
};
Header parse(std::istream& in) {
    char riff[4], wave[4];
    in.read(riff, 4);
    auto size = get32(in);
    in.read(wave, 4);
    if (std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE")
        throw std::runtime_error("not a RIFF/WAVE file");
    (void)size;
    Header h;
    bool fmt = false, data = false;
    while (in && (!fmt || !data)) {
        char id[4];
        in.read(id, 4);
        if (!in)
            break;
        auto n = get32(in);
        auto pos = in.tellg();
        std::string name(id, 4);
        if (name == "fmt ") {
            h.format = get16(in);
            h.info.channels = get16(in);
            h.info.sample_rate = get32(in);
            (void)get32(in);
            h.block = get16(in);
            h.info.bits_per_sample = get16(in);
            fmt = true;
            if (n > 16)
                in.seekg(pos + static_cast<std::streamoff>(n));
        } else if (name == "data") {
            h.data_bytes = n;
            h.data_pos = in.tellg();
            data = true;
            in.seekg(h.data_pos + static_cast<std::streamoff>(n));
        } else
            in.seekg(pos + static_cast<std::streamoff>(n));
        if (n & 1)
            in.seekg(in.tellg() + std::streamoff(1));
    }
    if (!fmt || !data || !h.info.channels || !h.block)
        throw std::runtime_error("invalid WAV header");
    h.info.floating_point = h.format == 3;
    h.info.frames = h.data_bytes / h.block;
    return h;
}
}  // namespace
void write_wav(const std::filesystem::path& path, const AudioBuffer& a, WavEncoding enc) {
    if (!a.channels || !a.sample_rate)
        throw std::invalid_argument("invalid audio");
    const std::uint16_t bits = enc == WavEncoding::Pcm16 ? 16 : enc == WavEncoding::Pcm24 ? 24 : 32;
    const bool fp = enc == WavEncoding::Float32;
    const auto bytes = bits / 8;
    if (a.samples.size() > (0xffffffffULL - 36) / bytes)
        throw std::length_error("classic WAV exceeds 4GB; use RF64 streaming writer");
    const std::uint64_t data64 = static_cast<std::uint64_t>(a.samples.size()) * bytes;
    std::ofstream o(path, std::ios::binary);
    if (!o)
        throw std::runtime_error("cannot create WAV");
    o.write("RIFF", 4);
    put32(o, static_cast<std::uint32_t>(data64 + 36));
    o.write("WAVEfmt ", 8);
    put32(o, 16);
    put16(o, fp ? 3 : 1);
    put16(o, a.channels);
    put32(o, a.sample_rate);
    put32(o, a.sample_rate * a.channels * bytes);
    put16(o, static_cast<std::uint16_t>(a.channels * bytes));
    put16(o, bits);
    o.write("data", 4);
    put32(o, static_cast<std::uint32_t>(data64));
    for (float f : a.samples) {
        float x = std::clamp(f, -1.0F, 1.0F);
        if (fp) {
            std::uint32_t u;
            static_assert(sizeof u == sizeof x);
            std::memcpy(&u, &x, 4);
            put32(o, u);
        } else if (bits == 16) {
            auto v = static_cast<std::int16_t>(std::lrint(x * 32767.0F));
            put16(o, static_cast<std::uint16_t>(v));
        } else if (bits == 24) {
            auto v = static_cast<std::int32_t>(std::lrint(x * 8388607.0F));
            for (int i = 0; i < 3; ++i)
                o.put(static_cast<char>(static_cast<std::uint32_t>(v) >> (i * 8)));
        } else {
            auto v = static_cast<std::int32_t>(std::lrint(x * 2147483647.0));
            put32(o, static_cast<std::uint32_t>(v));
        }
    }
    if (!o)
        throw std::runtime_error("failed writing WAV");
}
WavInfo inspect_wav(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open WAV");
    return parse(in).info;
}
AudioBuffer read_wav(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open WAV");
    auto h = parse(in);
    if (h.format != 1 && h.format != 3)
        throw std::runtime_error("unsupported WAV encoding");
    if (h.info.bits_per_sample != 8 && h.info.bits_per_sample != 16 && h.info.bits_per_sample != 24 &&
        h.info.bits_per_sample != 32)
        throw std::runtime_error("unsupported WAV bit depth");
    if (h.format == 3 && h.info.bits_per_sample != 32)
        throw std::runtime_error("only 32-bit float WAV supported");
    if (h.info.frames > std::numeric_limits<std::size_t>::max() / h.info.channels)
        throw std::length_error("WAV too large for memory; use streaming reader");
    AudioBuffer a(h.info.sample_rate, h.info.channels);
    a.samples.resize(static_cast<std::size_t>(h.info.frames) * h.info.channels);
    in.seekg(h.data_pos);
    for (auto& x : a.samples) {
        if (h.format == 3) {
            std::uint32_t u = get32(in);
            std::memcpy(&x, &u, 4);
        } else if (h.info.bits_per_sample == 8) {
            int c = in.get();
            if (c < 0)
                throw std::runtime_error("truncated WAV data");
            x = (static_cast<int>(c) - 128) / 128.0F;
        } else if (h.info.bits_per_sample == 16) {
            x = static_cast<std::int16_t>(get16(in)) / 32768.0F;
        } else if (h.info.bits_per_sample == 24) {
            unsigned char b[3];
            in.read(reinterpret_cast<char*>(b), 3);
            if (!in)
                throw std::runtime_error("truncated WAV data");
            std::int32_t v = b[0] | (b[1] << 8) | (b[2] << 16);
            if (v & 0x800000)
                v |= ~0xffffff;
            x = v / 8388608.0F;
        } else {
            x = static_cast<std::int32_t>(get32(in)) / 2147483648.0F;
        }
    }
    return a;
}
}  // namespace nd
