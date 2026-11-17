#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace nd {
struct TimedWord {
    std::string text;
    double start_seconds{};
    double end_seconds{};
    float confidence{1.0F};
};
struct ScriptWord {
    std::string text;
    std::size_t byte_start{};
    std::size_t byte_length{};
};
enum class AlignmentStatus { Matched, Missing, Repeated, Different };
struct WordMapping {
    std::size_t script_index{};
    std::size_t audio_index{};
    AlignmentStatus status{AlignmentStatus::Matched};
    double start_seconds{};
    double end_seconds{};
    float score{};
};
std::vector<ScriptWord> tokenize_script(const std::string& utf8);
std::vector<WordMapping> align_script(const std::vector<ScriptWord>& script,
                                      const std::vector<TimedWord>& audio, double fuzzy_threshold = 0.55);
}  // namespace nd
