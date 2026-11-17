#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audio_buffer.hpp"

namespace nd {
struct Clip {
    std::shared_ptr<const AudioBuffer> source;
    std::uint64_t source_start{};
    std::uint64_t length{};
    std::uint64_t timeline_start{};
    float gain{1.0F};
    bool muted{false};
    std::string name;
};
struct Marker {
    std::uint64_t frame{};
    std::string label;
};

class EditDecisionList {
   public:
    explicit EditDecisionList(std::uint32_t sample_rate = 48000, std::uint16_t channels = 1);
    void add_source(std::shared_ptr<const AudioBuffer> source, std::string name = {});
    void split(std::uint64_t frame);
    void erase(std::uint64_t start, std::uint64_t end, bool ripple = true);
    void silence(std::uint64_t start, std::uint64_t end);
    void trim(std::uint64_t start, std::uint64_t end);
    void duplicate(std::uint64_t start, std::uint64_t end);
    void join();
    void insert_silence(std::uint64_t frame, std::uint64_t length);
    void add_marker(std::uint64_t frame, std::string label);
    bool undo();
    bool redo();
    [[nodiscard]] AudioBuffer render() const;
    [[nodiscard]] std::uint64_t duration() const noexcept;
    [[nodiscard]] std::uint32_t sample_rate() const noexcept {
        return sample_rate_;
    }
    [[nodiscard]] std::uint16_t channels() const noexcept {
        return channels_;
    }
    [[nodiscard]] const std::vector<Clip>& clips() const noexcept {
        return clips_;
    }
    [[nodiscard]] const std::vector<Marker>& markers() const noexcept {
        return markers_;
    }
    [[nodiscard]] bool can_undo() const noexcept {
        return !undo_.empty();
    }
    [[nodiscard]] bool can_redo() const noexcept {
        return !redo_.empty();
    }

   private:
    struct State {
        std::vector<Clip> clips;
        std::vector<Marker> markers;
    };
    std::uint32_t sample_rate_;
    std::uint16_t channels_;
    std::vector<Clip> clips_;
    std::vector<Marker> markers_;
    std::vector<State> undo_, redo_;
    void save_undo();
    void normalize();
    void split_at(std::uint64_t frame);
};
}  // namespace nd
