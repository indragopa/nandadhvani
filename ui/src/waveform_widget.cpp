#include "waveform_widget.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

WaveformWidget::WaveformWidget(QWidget* p) : QWidget(p) {
    setMinimumHeight(250);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}
void WaveformWidget::setAudio(const nd::AudioBuffer* a) {
    audio_ = a;
    selection_start_ = selection_end_ = playhead_ = cursor_ = view_start_ = 0;
    update();
}
void WaveformWidget::setSelection(std::uint64_t s, std::uint64_t e) {
    selection_start_ = s;
    selection_end_ = e;
    update();
}
void WaveformWidget::setPlayhead(std::uint64_t f) {
    playhead_ = f;
    update();
}
std::uint64_t WaveformWidget::frameAt(int x) const {
    if (!audio_ || audio_->frames() == 0)
        return 0;
    double visible = static_cast<double>(audio_->frames()) / zoom_;
    double frame = static_cast<double>(view_start_) +
                   (static_cast<double>(std::clamp(x, 0, width())) / std::max(1, width())) * visible;
    return static_cast<std::uint64_t>(std::clamp(frame, 0.0, static_cast<double>(audio_->frames())));
}
void WaveformWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#10161d"));
    p.setRenderHint(QPainter::Antialiasing, false);
    const int mid = height() / 2;
    const int left = 60;
    QRect area(left, 30, std::max(1, width() - left - 16), std::max(1, height() - 55));
    p.fillRect(area, QColor("#151e27"));
    p.setPen(QPen(QColor("#253440"), 1));
    for (int k = 0; k <= 4; ++k) {
        int y = area.top() + k * area.height() / 4;
        p.drawLine(area.left(), y, area.right(), y);
    }
    p.setPen(QColor("#8395a7"));
    p.drawText(10, 18, "00:00");
    if (audio_ && audio_->frames()) {
        double seconds = audio_->frames() / static_cast<double>(audio_->sample_rate);
        p.drawText(width() - 85, 18, QString::number(seconds, 'f', 1) + " s");
    }
    if (selection_end_ > selection_start_ && audio_) {
        double visible = static_cast<double>(audio_->frames()) / zoom_;
        auto xx = [&](std::uint64_t f) {
            return area.left() +
                   static_cast<int>((static_cast<double>(f) - view_start_) / visible * area.width());
        };
        QRect r(xx(selection_start_), area.top(), std::max(1, xx(selection_end_) - xx(selection_start_)),
                area.height());
        p.fillRect(r, QColor(49, 139, 142, 65));
    }
    if (audio_ && !audio_->samples.empty()) {
        p.setPen(QPen(QColor("#48c4b8"), 1));
        double visible = static_cast<double>(audio_->frames()) / zoom_;
        std::uint64_t first = view_start_,
                      last = std::min<std::uint64_t>(audio_->frames(),
                                                     view_start_ + static_cast<std::uint64_t>(visible));
        const int n = area.width();
        for (int x = 0; x < n; ++x) {
            auto f0 = first + static_cast<std::uint64_t>((static_cast<double>(x) / n) * (last - first));
            auto f1 = first + static_cast<std::uint64_t>((static_cast<double>(x + 1) / n) * (last - first));
            if (f1 <= f0)
                f1 = std::min<std::uint64_t>(f0 + 1, audio_->frames());
            float lo = 1, hi = -1;
            for (auto f = f0; f < f1 && f < audio_->frames(); ++f)
                for (std::size_t c = 0; c < audio_->channels; ++c) {
                    float v = audio_->samples[static_cast<std::size_t>(f * audio_->channels + c)];
                    lo = std::min(lo, v);
                    hi = std::max(hi, v);
                }
            int y1 = mid - static_cast<int>(hi * area.height() * .45),
                y2 = mid - static_cast<int>(lo * area.height() * .45);
            p.drawLine(area.left() + x, y1, area.left() + x, y2);
        }
    }
    if (audio_) {
        double visible = static_cast<double>(audio_->frames()) / zoom_;
        int px = area.left() +
                 static_cast<int>((static_cast<double>(playhead_) - view_start_) / visible * area.width());
        p.setPen(QPen(QColor("#f8b84e"), 2));
        p.drawLine(px, area.top(), px, area.bottom());
    }
    p.setPen(QColor("#91a1b1"));
    p.drawText(12, area.center().y(), "Voice");
    p.setPen(QColor("#35434e"));
    p.drawRect(area);
}
void WaveformWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        cursor_ = frameAt(e->position().x() - 60);
        selection_start_ = cursor_;
        selection_end_ = cursor_;
        dragging_ = true;
        emit cursorMoved(cursor_);
        emit selectionChanged();
        update();
    }
}
void WaveformWidget::mouseMoveEvent(QMouseEvent* e) {
    if (dragging_) {
        auto f = frameAt(e->position().x() - 60);
        selection_start_ = std::min(cursor_, f);
        selection_end_ = std::max(cursor_, f);
        emit selectionChanged();
        update();
    }
}
void WaveformWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = false;
        auto f = frameAt(e->position().x() - 60);
        if (f == cursor_) {
            selection_start_ = selection_end_ = f;
        }
        emit cursorMoved(f);
        emit selectionChanged();
        update();
    }
}
void WaveformWidget::wheelEvent(QWheelEvent* e) {
    if (e->angleDelta().y() > 0)
        zoom_ = std::min(zoom_ * 1.25, 1000.0);
    else
        zoom_ = std::max(1.0, zoom_ / 1.25);
    update();
}
