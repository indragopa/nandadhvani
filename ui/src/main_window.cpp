#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <filesystem>

#include "nandadhvani/core/wav.hpp"
#include "waveform_widget.hpp"

namespace {
std::filesystem::path nativePath(const QString& value) {
    const auto utf8 = value.toUtf8();
    return std::filesystem::u8path(utf8.constData());
}
std::string utf8Path(const std::filesystem::path& value) {
    const auto utf8 = value.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}
}  // namespace

MainWindow::MainWindow(QWidget* p) : QMainWindow(p) {
    setWindowTitle("NandaDhvani");
    resize(1320, 820);
    buildUi();
    buildMenus();
    timer_.setInterval(30);
    connect(&timer_, &QTimer::timeout, this, &MainWindow::updateTransport);
    QSettings settings("NandaDhvani", "NandaDhvani");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    splitter_->restoreState(settings.value("splitter").toByteArray());
}
MainWindow::~MainWindow() {
    QSettings settings("NandaDhvani", "NandaDhvani");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("splitter", splitter_->saveState());
}
void MainWindow::buildUi() {
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(10, 8, 10, 6);
    layout->setSpacing(8);
    auto* bar = new QHBoxLayout();
    record_ = new QPushButton("●  Record", root);
    record_->setObjectName("recordButton");
    record_->setMinimumSize(120, 44);
    play_ = new QPushButton("▶  Play", root);
    play_->setMinimumSize(105, 44);
    auto* pause = new QPushButton("Ⅱ  Pause", root);
    pause->setMinimumSize(100, 44);
    auto* stop = new QPushButton("■  Stop", root);
    stop->setMinimumSize(95, 44);
    auto* start = new QPushButton("|◀", root);
    auto* end = new QPushButton("▶|", root);
    time_ = new QLabel("00:00:00.000", root);
    time_->setMinimumWidth(140);
    time_->setStyleSheet("font: 18px monospace; color:#dce8f0;");
    bar->addWidget(record_);
    bar->addWidget(play_);
    bar->addWidget(pause);
    bar->addWidget(stop);
    bar->addWidget(start);
    bar->addWidget(end);
    bar->addSpacing(16);
    bar->addWidget(time_);
    bar->addStretch();
    auto* mic = new QLabel("Mic: System default (not connected)", root);
    mic->setToolTip("Audio capture is not yet enabled in this build");
    auto* out = new QLabel("Output: System default", root);
    bar->addWidget(mic);
    bar->addSpacing(16);
    bar->addWidget(out);
    layout->addLayout(bar);
    splitter_ = new QSplitter(Qt::Horizontal, root);
    script_ = new QTextEdit(splitter_);
    script_->setReadOnly(true);
    script_->setPlaceholderText(
        "Open a script with File → Open Script…\n\nUnicode text is preserved. Use the waveform pane to "
        "inspect and edit imported audio.");
    script_->setAcceptRichText(false);
    script_->setFont(QFont("Noto Sans", 13));
    script_->setAccessibleName("Read-along script");
    waveform_ = new WaveformWidget(splitter_);
    waveform_->setAccessibleName("Audio waveform and selection");
    splitter_->addWidget(script_);
    splitter_->addWidget(waveform_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 2);
    layout->addWidget(splitter_, 1);
    auto* bottom = new QHBoxLayout();
    auto* mixerLabel =
        new QLabel("Mixer  ·  Voice  [ −∞ dB ]  Pan 0  Mute  Solo      Master  [ −∞ dB ]", root);
    mixerLabel->setStyleSheet(
        "padding:12px;background:#151e27;border:1px solid #293845;border-radius:5px;color:#aab8c5;");
    bottom->addWidget(mixerLabel, 1);
    layout->addLayout(bottom);
    setCentralWidget(root);
    status_ = new QLabel("48 kHz · 32-bit float · No audio loaded", this);
    statusBar()->addWidget(status_, 1);
    record_->setToolTip("Record — audio capture backend is not yet configured in this build");
    play_->setToolTip("Play / Pause — audio output backend is not connected in this preview");
    connect(record_, &QPushButton::clicked, this, &MainWindow::toggleRecord);
    connect(play_, &QPushButton::clicked, this, &MainWindow::playPause);
    connect(pause, &QPushButton::clicked, this, &MainWindow::playPause);
    connect(stop, &QPushButton::clicked, this, &MainWindow::stopPlayback);
    connect(start, &QPushButton::clicked, this, [this] {
        playhead_ = 0;
        waveform_->setPlayhead(0);
        updateTransport();
    });
    connect(end, &QPushButton::clicked, this, [this] {
        playhead_ = audio_ ? audio_->frames() : 0;
        waveform_->setPlayhead(playhead_);
        updateTransport();
    });
    connect(waveform_, &WaveformWidget::selectionChanged, this, &MainWindow::syncSelection);
    connect(waveform_, &WaveformWidget::cursorMoved, this, [this](std::uint64_t f) { playhead_ = f; });
}
void MainWindow::buildMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto* openS = file->addAction("Open &Script…");
    openS->setShortcut(QKeySequence("Ctrl+O"));
    connect(openS, &QAction::triggered, this, &MainWindow::openScript);
    auto* openA = file->addAction("Import &Audio…");
    openA->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openA, &QAction::triggered, this, &MainWindow::openAudio);
    file->addSeparator();
    auto* save = file->addAction("Save Project…");
    save->setShortcut(QKeySequence("Ctrl+S"));
    connect(save, &QAction::triggered, this, &MainWindow::saveProject);
    auto* exportW = file->addAction("Export WAV…");
    connect(exportW, &QAction::triggered, this, [this] {
        if (!edl_)
            return;
        auto p = QFileDialog::getSaveFileName(this, "Export WAV", {}, "WAV audio (*.wav)");
        if (p.isEmpty())
            return;
        try {
            nd::write_wav(nativePath(p), edl_->render());
            status_->setText("Exported WAV: " + p);
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Export failed", e.what());
        }
    });
    file->addSeparator();
    auto* quit = file->addAction("E&xit");
    quit->setShortcut(QKeySequence("Alt+F4"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);
    auto* edit = menuBar()->addMenu("&Edit");
    auto* undo = edit->addAction("Undo");
    undo->setShortcut(QKeySequence("Ctrl+Z"));
    connect(undo, &QAction::triggered, this, &MainWindow::undoEdit);
    auto* redo = edit->addAction("Redo");
    redo->setShortcut(QKeySequence("Ctrl+Y"));
    connect(redo, &QAction::triggered, this, &MainWindow::redoEdit);
    edit->addSeparator();
    auto* split = edit->addAction("Split at cursor");
    split->setShortcut(QKeySequence("S"));
    connect(split, &QAction::triggered, this, &MainWindow::splitAtCursor);
    auto* del = edit->addAction("Delete selection (ripple)");
    del->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(del, &QAction::triggered, this, &MainWindow::deleteSelection);
    auto* sil = edit->addAction("Silence selection");
    sil->setShortcut(QKeySequence("Ctrl+L"));
    connect(sil, &QAction::triggered, this, [this] {
        if (edl_ && waveform_->selectionEnd() > waveform_->selectionStart()) {
            edl_->silence(waveform_->selectionStart(), waveform_->selectionEnd());
            refreshEdl();
        }
    });
    auto* view = menuBar()->addMenu("&View");
    auto* swap = view->addAction("Swap script / waveform sides");
    swap->setCheckable(true);
    connect(swap, &QAction::toggled, this, [this](bool) {
        auto sizes = splitter_->sizes();
        QWidget* a = splitter_->widget(0);
        QWidget* b = splitter_->widget(1);
        splitter_->replaceWidget(0, b);
        splitter_->replaceWidget(1, a);
        splitter_->setSizes(sizes);
    });
    auto* stack = view->addAction("Stacked layout");
    stack->setCheckable(true);
    connect(stack, &QAction::toggled, this,
            [this](bool on) { splitter_->setOrientation(on ? Qt::Vertical : Qt::Horizontal); });
    auto* help = menuBar()->addMenu("&Help");
    auto* about = help->addAction("About NandaDhvani");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this, "About NandaDhvani",
            "NandaDhvani — audiobook and podcast studio\n\nCore editing, WAV import/export, and script "
            "display are available in this preview. Audio device recording and additional import/export "
            "codecs are not yet wired.\n\nNo telemetry. No network calls.");
    });
    auto* keys = help->addAction("Keyboard shortcuts");
    keys->setShortcut(QKeySequence(Qt::Key_F1));
    connect(keys, &QAction::triggered, this, [this] {
        QMessageBox::information(
            this, "Keyboard shortcuts",
            "S  Split at cursor\nDelete  Ripple delete selection\nCtrl+L  Silence selection\nCtrl+Z / Ctrl+Y "
            " Undo / Redo\nCtrl+O  Open script\nCtrl+Shift+O  Import audio\nCtrl+S  Save project\n\nAudio "
            "transport shortcuts are not active in this preview.");
    });
}
void MainWindow::openScript() {
    auto p = QFileDialog::getOpenFileName(
        this, "Open script", {},
        "Scripts (*.txt *.md *.markdown *.html *.htm *.rtf *.docx *.odt *.pdf *.epub);;All files (*.*)");
    if (p.isEmpty())
        return;
    try {
        document_ = nd::io::open_script(nativePath(p));
        script_->setPlainText(
            QString::fromUtf8(document_.text.data(), static_cast<qsizetype>(document_.text.size())));
        project_.script_path = utf8Path(nativePath(p));
        project_.title = QFileInfo(p).completeBaseName().toStdString();
        status_->setText(QString("Script loaded · %1 characters · %2 chapters")
                             .arg(document_.text.size())
                             .arg(document_.chapters.size()));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Could not open script", e.what());
    }
}
void MainWindow::openAudio() {
    auto p = QFileDialog::getOpenFileName(this, "Import audio", {}, "WAV audio (*.wav);;All files (*.*)");
    if (p.isEmpty())
        return;
    try {
        auto a = nd::read_wav(nativePath(p));
        setAudio(std::move(a), QFileInfo(p).fileName());
        project_.tracks.clear();
        nd::TrackInfo t;
        t.name = QFileInfo(p).completeBaseName().toStdString();
        t.media = nativePath(p);
        project_.tracks.push_back(t);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Could not import audio", e.what());
    }
}
void MainWindow::setAudio(nd::AudioBuffer a, const QString& label) {
    audio_ = std::make_shared<nd::AudioBuffer>(std::move(a));
    edl_ = std::make_unique<nd::EditDecisionList>(audio_->sample_rate, audio_->channels);
    edl_->add_source(audio_, label.toStdString());
    waveform_->setAudio(audio_.get());
    project_.sample_rate = audio_->sample_rate;
    project_.channels = audio_->channels;
    playhead_ = 0;
    updateStatus();
}
void MainWindow::refreshEdl() {
    if (!edl_)
        return;
    audio_ = std::make_shared<nd::AudioBuffer>(edl_->render());
    waveform_->setAudio(audio_.get());
    playhead_ = std::min<std::uint64_t>(playhead_, audio_->frames());
    waveform_->setPlayhead(playhead_);
    updateStatus();
}
void MainWindow::saveProject() {
    auto p =
        QFileDialog::getSaveFileName(this, "Save project", QString::fromStdString(project_.title) + ".ndproj",
                                     "NandaDhvani project (*.ndproj)");
    if (p.isEmpty())
        return;
    try {
        nd::save_project(nativePath(p), project_);
        status_->setText("Saved project: " + p);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Save failed", e.what());
    }
}
void MainWindow::playPause() {
    status_->setText(audio_ ? "Playback backend is not connected in this build; waveform editing and WAV "
                              "export are available."
                            : "Import a WAV file first (File → Import Audio…)");
}
void MainWindow::stopPlayback() {
    playhead_ = 0;
    waveform_->setPlayhead(0);
    if (time_)
        time_->setText("00:00:00.000");
}
void MainWindow::toggleRecord() {
    QMessageBox::information(this, "Recording unavailable",
                             "The audio capture backend is not yet connected in this build. You can import "
                             "and edit WAV files using File → Import Audio…");
}
void MainWindow::splitAtCursor() {
    if (!edl_) {
        return;
    }
    try {
        edl_->split(waveform_->cursorFrame());
        status_->setText("Split at cursor");
    } catch (const std::exception& e) {
        status_->setText(e.what());
    }
}
void MainWindow::deleteSelection() {
    if (!edl_)
        return;
    auto s = waveform_->selectionStart(), e = waveform_->selectionEnd();
    if (e <= s)
        return;
    try {
        edl_->erase(s, e, true);
        refreshEdl();
        status_->setText("Deleted selection (ripple)");
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Edit failed", ex.what());
    }
}
void MainWindow::undoEdit() {
    if (edl_ && edl_->undo())
        refreshEdl();
}
void MainWindow::redoEdit() {
    if (edl_ && edl_->redo())
        refreshEdl();
}
void MainWindow::updateTransport() {
    if (!audio_)
        return;
    const double seconds = static_cast<double>(playhead_) / audio_->sample_rate;
    const int hours = static_cast<int>(seconds / 3600);
    const int minutes = static_cast<int>(seconds / 60) % 60;
    const double remain = seconds - static_cast<int>(seconds / 60) * 60;
    time_->setText(QString("%1:%2:%3")
                       .arg(hours, 2, 10, QChar('0'))
                       .arg(minutes, 2, 10, QChar('0'))
                       .arg(remain, 6, 'f', 3, QChar('0')));
}
void MainWindow::syncSelection() {
    if (waveform_->selectionEnd() > waveform_->selectionStart())
        status_->setText(QString("Selection: %1 – %2 frames")
                             .arg(waveform_->selectionStart())
                             .arg(waveform_->selectionEnd()));
}
void MainWindow::updateStatus() {
    if (audio_)
        status_->setText(QString("%1 Hz · %2-bit float · %3 channels · %4 frames")
                             .arg(audio_->sample_rate)
                             .arg(32)
                             .arg(audio_->channels)
                             .arg(audio_->frames()));
}
