#include "nandadhvani/core/edl.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace nd {
EditDecisionList::EditDecisionList(std::uint32_t rate, std::uint16_t channels)
    : sample_rate_(rate), channels_(channels) {
    if (!rate || !channels)
        throw std::invalid_argument("invalid EDL format");
}
void EditDecisionList::save_undo() {
    undo_.push_back({clips_, markers_});
    redo_.clear();
}
std::uint64_t EditDecisionList::duration() const noexcept {
    std::uint64_t end = 0;
    for (const auto& c : clips_)
        end = std::max(end, c.timeline_start + c.length);
    return end;
}
void EditDecisionList::normalize() {
    clips_.erase(std::remove_if(clips_.begin(), clips_.end(), [](const Clip& c) { return !c.length; }),
                 clips_.end());
    std::stable_sort(clips_.begin(), clips_.end(),
                     [](const Clip& a, const Clip& b) { return a.timeline_start < b.timeline_start; });
    for (auto& c : clips_)
        if (c.source && c.source_start + c.length > c.source->frames())
            c.length = c.source->frames() - c.source_start;
}
void EditDecisionList::add_source(std::shared_ptr<const AudioBuffer> source, std::string name) {
    if (!source || source->sample_rate != sample_rate_ || source->channels != channels_)
        throw std::invalid_argument("source format mismatch");
    if (source->frames() == 0)
        return;
    save_undo();
    clips_.push_back({source, 0, source->frames(), duration(), 1.0F, false, std::move(name)});
}
void EditDecisionList::split_at(std::uint64_t frame) {
    for (std::size_t i = 0; i < clips_.size(); ++i) {
        auto c = clips_[i];
        if (frame > c.timeline_start && frame < c.timeline_start + c.length) {
            const auto left = frame - c.timeline_start;
            clips_[i].length = left;
            c.source_start += left;
            c.length -= left;
            c.timeline_start = frame;
            clips_.insert(clips_.begin() + static_cast<std::ptrdiff_t>(i + 1), std::move(c));
            return;
        }
    }
}
void EditDecisionList::split(std::uint64_t frame) {
    if (frame > duration())
        throw std::out_of_range("split position");
    save_undo();
    split_at(frame);
}
void EditDecisionList::erase(std::uint64_t start, std::uint64_t end, bool ripple) {
    if (start > end || end > duration())
        throw std::out_of_range("delete range");
    if (start == end)
        return;
    save_undo();
    split_at(start);
    split_at(end);
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
                       [&](const Clip& c) { return c.timeline_start >= start && c.timeline_start < end; }),
        clips_.end());
    if (ripple) {
        const auto len = end - start;
        for (auto& c : clips_)
            if (c.timeline_start >= end)
                c.timeline_start -= len;
        for (auto& m : markers_)
            if (m.frame >= end)
                m.frame -= len;
            else if (m.frame > start)
                m.frame = start;
    }
    normalize();
}
void EditDecisionList::silence(std::uint64_t start, std::uint64_t end) {
    if (start > end || end > duration())
        throw std::out_of_range("silence range");
    if (start == end)
        return;
    save_undo();
    split_at(start);
    split_at(end);
    for (auto& c : clips_)
        if (c.timeline_start >= start && c.timeline_start < end)
            c.muted = true;
}
void EditDecisionList::trim(std::uint64_t start, std::uint64_t end) {
    if (start >= end || end > duration())
        throw std::out_of_range("trim range");
    save_undo();
    split_at(start);
    split_at(end);
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
                       [&](const Clip& c) { return c.timeline_start < start || c.timeline_start >= end; }),
        clips_.end());
    for (auto& c : clips_)
        c.timeline_start -= start;
    for (auto& m : markers_) {
        if (m.frame < start)
            m.frame = 0;
        else
            m.frame = std::min(m.frame - start, end - start);
    }
    normalize();
}
void EditDecisionList::duplicate(std::uint64_t start, std::uint64_t end) {
    if (start >= end || end > duration())
        throw std::out_of_range("duplicate range");
    save_undo();
    split_at(start);
    split_at(end);
    std::vector<Clip> copy;
    for (const auto& c : clips_)
        if (c.timeline_start >= start && c.timeline_start < end) {
            auto d = c;
            d.timeline_start += duration() - start;
            copy.push_back(std::move(d));
        }
    clips_.insert(clips_.end(), copy.begin(), copy.end());
    normalize();
}
void EditDecisionList::join() {
    const auto can_merge = [](const Clip& left, const Clip& right) {
        return left.timeline_start + left.length == right.timeline_start && left.source == right.source &&
               left.source_start + left.length == right.source_start && left.gain == right.gain &&
               left.muted == right.muted;
    };
    bool has_merge = false;
    for (std::size_t i = 1; i < clips_.size(); ++i)
        has_merge = has_merge || can_merge(clips_[i - 1], clips_[i]);
    if (!has_merge)
        return;
    save_undo();
    for (std::size_t i = 1; i < clips_.size();) {
        auto& left = clips_[i - 1];
        const auto& right = clips_[i];
        if (can_merge(left, right)) {
            left.length += right.length;
            clips_.erase(clips_.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}
void EditDecisionList::insert_silence(std::uint64_t frame, std::uint64_t length) {
    if (frame > duration())
        throw std::out_of_range("insert position");
    if (!length)
        return;
    if (length > std::numeric_limits<std::uint64_t>::max() - duration() ||
        length > std::numeric_limits<std::size_t>::max() / channels_)
        throw std::length_error("inserted silence is too large");
    save_undo();
    split_at(frame);
    for (auto& c : clips_)
        if (c.timeline_start >= frame)
            c.timeline_start += length;
    auto silence = std::make_shared<AudioBuffer>(sample_rate_, channels_);
    silence->samples.resize(static_cast<std::size_t>(length) * channels_, 0.0F);
    clips_.push_back({silence, 0, length, frame, 1.0F, false, "Silence"});
    normalize();
}
void EditDecisionList::add_marker(std::uint64_t frame, std::string label) {
    if (frame > duration())
        throw std::out_of_range("marker position");
    save_undo();
    markers_.push_back({frame, std::move(label)});
    std::stable_sort(markers_.begin(), markers_.end(), [](auto& a, auto& b) { return a.frame < b.frame; });
}
bool EditDecisionList::undo() {
    if (undo_.empty())
        return false;
    redo_.push_back({clips_, markers_});
    auto s = std::move(undo_.back());
    undo_.pop_back();
    clips_ = std::move(s.clips);
    markers_ = std::move(s.markers);
    return true;
}
bool EditDecisionList::redo() {
    if (redo_.empty())
        return false;
    undo_.push_back({clips_, markers_});
    auto s = std::move(redo_.back());
    redo_.pop_back();
    clips_ = std::move(s.clips);
    markers_ = std::move(s.markers);
    return true;
}
AudioBuffer EditDecisionList::render() const {
    if (duration() > std::numeric_limits<std::size_t>::max() / channels_)
        throw std::length_error("render too large");
    AudioBuffer out(sample_rate_, channels_);
    out.samples.assign(static_cast<std::size_t>(duration()) * channels_, 0.0F);
    for (const auto& c : clips_)
        if (c.source && !c.muted) {
            for (std::uint64_t i = 0; i < c.length; ++i) {
                const auto dst = (c.timeline_start + i) * channels_;
                const auto src = (c.source_start + i) * channels_;
                for (std::size_t ch = 0; ch < channels_; ++ch)
                    out.samples[static_cast<std::size_t>(dst + ch)] +=
                        c.source->samples[static_cast<std::size_t>(src + ch)] * c.gain;
            }
        }
    return out;
}
}  // namespace nd
