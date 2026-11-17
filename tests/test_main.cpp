#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "nandadhvani/core/alignment.hpp"
#include "nandadhvani/core/dsp.hpp"
#include "nandadhvani/core/edl.hpp"
#include "nandadhvani/core/project.hpp"
#include "nandadhvani/core/realtime.hpp"
#include "nandadhvani/core/recording.hpp"
#include "nandadhvani/core/wav.hpp"
#include "nandadhvani/io/script.hpp"

namespace {
int failures = 0;
#define CHECK(x)                                                                      \
    do {                                                                              \
        if (!(x)) {                                                                   \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #x << "\n"; \
            ++failures;                                                               \
        }                                                                             \
    } while (false)
template <class F>
void throws(F f) {
    bool yes = false;
    try {
        f();
    } catch (...) {
        yes = true;
    }
    CHECK(yes);
}
void test_audio() {
    nd::AudioBuffer b(48000, 1);
    float x[] = {.25F, -.5F, 1.0F};
    b.append_frames(x, 3);
    CHECK(b.frames() == 3);
    CHECK(b.at(1, 0) == -.5F);
    auto s = b.slice(1, 2);
    CHECK(s.frames() == 2 && s.at(0, 0) == -.5F);
    throws([&] { (void)b.at(3, 0); });
}
void test_edl() {
    auto a = std::make_shared<nd::AudioBuffer>(48000, 1);
    float x[] = {1, 2, 3, 4, 5, 6};
    a->append_frames(x, 6);
    nd::EditDecisionList e;
    e.add_source(a, "voice");
    e.split(3);
    CHECK(e.clips().size() == 2);
    e.join();
    CHECK(e.clips().size() == 1);
    CHECK(e.undo());
    CHECK(e.clips().size() == 2);
    auto r = e.render();
    CHECK(r.samples == a->samples);
    e.erase(2, 4, true);
    r = e.render();
    CHECK(r.frames() == 4);
    CHECK(r.samples[0] == 1 && r.samples[1] == 2 && r.samples[2] == 5 && r.samples[3] == 6);
    CHECK(e.undo());
    CHECK(e.render().samples == a->samples);
    CHECK(e.redo());
    CHECK(e.render().frames() == 4);
    e.insert_silence(1, 2);
    CHECK(e.render().frames() == 6);
    e.add_marker(0, "start");
    CHECK(e.markers().size() == 1);
}
void test_wav() {
    auto p = std::filesystem::temp_directory_path() / "nd_core_test.wav";
    nd::AudioBuffer b(44100, 2);
    for (int i = 0; i < 200; ++i) {
        b.samples.push_back(std::sin(i * .1F));
        b.samples.push_back(std::cos(i * .1F));
    }
    nd::write_wav(p, b, nd::WavEncoding::Float32);
    auto info = nd::inspect_wav(p);
    CHECK(info.frames == 200 && info.channels == 2 && info.floating_point);
    auto c = nd::read_wav(p);
    CHECK(c.samples.size() == b.samples.size());
    CHECK(std::abs(c.samples[33] - b.samples[33]) < 1e-6);
    nd::write_wav(p, b, nd::WavEncoding::Pcm24);
    auto d = nd::read_wav(p);
    CHECK(d.frames() == b.frames());
    CHECK(std::abs(d.samples[33] - b.samples[33]) < 1e-4);
    std::filesystem::remove(p);
}
void test_dsp() {
    nd::AudioBuffer b(48000, 1);
    for (int i = 0; i < 48000; ++i)
        b.samples.push_back(.1F * std::sin(2 * 3.14159265F * 1000 * i / 48000));
    auto before = nd::measure(b);
    CHECK(before.rms_db < -18 && before.rms_db > -25);
    nd::normalize_peak(b, -6);
    CHECK(std::abs(nd::measure(b).peak_db + 6) < .01);
    auto f = nd::make_lowpass(48000, 3000);
    nd::apply_biquad(b, f);
    nd::fade(b, true, 500);
    CHECK(b.samples[0] == 0);
    nd::remove_hum(b, 50, 20, 3);
}
void test_script() {
    std::string devan = "ಕನ್ನಡ ā ī ū ṛ ṣ ṇ ś ṃ ḥ\n";
    auto d = nd::io::decode_script(devan, ".md");
    CHECK(d.text == devan);
    CHECK(d.text.find("ā ī ū ṛ ṣ ṇ ś ṃ ḥ") != std::string::npos);
    std::string utf16;
    utf16.push_back(char(0xff));
    utf16.push_back(char(0xfe));
    utf16.push_back('A');
    utf16.push_back(0);
    auto u = nd::io::decode_script(utf16, ".txt");
    CHECK(u.text == "A" && u.converted_encoding);
    auto h = nd::io::decode_script("<p>Hello</p>", ".html");
    CHECK(h.text == "Hello");
    throws([] { (void)nd::io::decode_script("x", ".docx"); });
}
void test_alignment() {
    auto s = nd::tokenize_script("This is a simple line");
    std::vector<nd::TimedWord> a = {
        {"This", 0, .2, 1}, {"is", .2, .3, 1}, {"a", .3, .4, 1}, {"simple", .4, .8, 1}, {"line", .8, 1, 1}};
    auto m = nd::align_script(s, a);
    CHECK(m.size() == s.size());
    for (auto& x : m)
        CHECK(x.status == nd::AlignmentStatus::Matched);
    std::vector<nd::TimedWord> short_a = {{"This", 0, .2, 1}};
    auto missing = nd::align_script(s, short_a);
    bool saw = false;
    for (auto& x : missing)
        saw |= x.status == nd::AlignmentStatus::Missing;
    CHECK(saw);
}
void test_realtime() {
    nd::SpscRingBuffer<int, 8> q;
    CHECK(q.size() == 0);
    for (int i = 0; i < 7; ++i)
        CHECK(q.try_push(i));
    CHECK(!q.try_push(8));
    for (int i = 0; i < 7; ++i) {
        int v = -1;
        CHECK(q.try_pop(v));
        CHECK(v == i);
    }
    int v{};
    CHECK(!q.try_pop(v));
}
void test_stream_writer() {
    auto p = std::filesystem::temp_directory_path() / "nd_stream_test.wav";
    {
        nd::CrashSafeWavWriter w(p, 48000, 1, 2);
        float x[] = {.125F, -.25F, .5F};
        w.append(x, 2);
        CHECK(w.frames_written() == 2);
        w.flush();
        w.append(x + 2, 1);
        w.finish();
    }
    auto a = nd::read_wav(p);
    CHECK(a.frames() == 3);
    CHECK(std::abs(a.samples[0] - .125F) < 1e-6F);
    CHECK(std::abs(a.samples[2] - .5F) < 1e-6F);
    std::filesystem::remove(p);
}
void test_project() {
    auto p = std::filesystem::temp_directory_path() / "nd_core_test.ndproj";
    nd::ProjectData x;
    x.title = "Nanda \"test\"";
    x.script_path = "કನ್ನಡ.md";
    x.sample_rate = 44100;
    x.tracks.push_back({"Voice", "Voice", "#fff", .8F, -.2F, false, false, false, true, "voice.wav"});
    x.markers.push_back({48000, "chapter one"});
    nd::save_project(p, x);
    auto y = nd::load_project(p);
    CHECK(y.title == x.title && y.script_path == x.script_path && y.sample_rate == 44100);
    CHECK(y.tracks.size() == 1 && y.tracks[0].name == "Voice" && y.tracks[0].armed);
    CHECK(y.markers.size() == 1 && y.markers[0].frame == 48000);
    std::filesystem::remove(p);
}
}  // namespace
int main() {
    test_audio();
    test_edl();
    test_wav();
    test_dsp();
    test_script();
    test_alignment();
    test_realtime();
    test_stream_writer();
    test_project();
    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All NandaDhvani core tests passed.\n";
    return 0;
}
