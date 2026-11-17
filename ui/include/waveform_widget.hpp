#pragma once
#include <QWidget>

#include "nandadhvani/core/audio_buffer.hpp"
class WaveformWidget final : public QWidget {
    Q_OBJECT
   public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    void setAudio(const nd::AudioBuffer* audio);
    void setSelection(std::uint64_t start, std::uint64_t end);
    void setPlayhead(std::uint64_t frame);
    [[nodiscard]] std::uint64_t selectionStart() const {
        return selection_start_;
    }
    [[nodiscard]] std::uint64_t selectionEnd() const {
        return selection_end_;
    }
    [[nodiscard]] std::uint64_t cursorFrame() const {
        return cursor_;
    }
   signals:
    void selectionChanged();
    void cursorMoved(std::uint64_t frame);

   protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

   private:
    const nd::AudioBuffer* audio_{};
    std::uint64_t selection_start_{}, selection_end_{}, playhead_{}, cursor_{};
    bool dragging_{};
    double zoom_{1.0};
    std::uint64_t view_start_{};
    std::uint64_t frameAt(int x) const;
};
