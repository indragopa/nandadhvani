#pragma once
#include <QMainWindow>
#include <QTimer>
#include <memory>

#include "nandadhvani/core/edl.hpp"
#include "nandadhvani/core/project.hpp"
#include "nandadhvani/io/script.hpp"
class QTextEdit;
class QSplitter;
class QLabel;
class QPushButton;
class QSlider;
class QTreeWidget;
class WaveformWidget;
class MainWindow final : public QMainWindow {
    Q_OBJECT
   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
   private slots:
    void openScript();
    void openAudio();
    void saveProject();
    void playPause();
    void stopPlayback();
    void toggleRecord();
    void splitAtCursor();
    void deleteSelection();
    void undoEdit();
    void redoEdit();
    void updateTransport();
    void syncSelection();

   private:
    void buildUi();
    void buildMenus();
    void setAudio(nd::AudioBuffer audio, const QString& label);
    void refreshEdl();
    void updateStatus();
    QTextEdit* script_{};
    QSplitter* splitter_{};
    WaveformWidget* waveform_{};
    QLabel* time_{};
    QLabel* status_{};
    QPushButton* record_{};
    QPushButton* play_{};
    QTimer timer_;
    std::shared_ptr<nd::AudioBuffer> audio_;
    std::unique_ptr<nd::EditDecisionList> edl_;
    nd::io::ScriptDocument document_;
    nd::ProjectData project_;
    std::uint64_t playhead_{};
    bool playing_{};
    bool recording_{};
};
