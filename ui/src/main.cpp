#include <QApplication>
#include <QFontDatabase>
#include <QStyleFactory>

#include "main_window.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("NandaDhvani");
    QApplication::setOrganizationName("NandaDhvani");
    QApplication::setApplicationVersion(ND_VERSION);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(R"(
QWidget { background:#111820; color:#d7e0e8; font-family:"Segoe UI", "Noto Sans", sans-serif; font-size:10pt; }
QMenuBar,QMenu,QToolBar { background:#18232d; color:#d7e0e8; border:0; }
QMenuBar::item:selected,QMenu::item:selected { background:#29404c; }
QPushButton { background:#243541; border:1px solid #405261; border-radius:5px; padding:7px 12px; color:#e3ebf1; }
QPushButton:hover { background:#304a58; } QPushButton:pressed { background:#1c8a83; }
QPushButton#recordButton { background:#b93b46; border:1px solid #e15d69; font-weight:bold; font-size:12pt; }
QPushButton#recordButton:hover { background:#d44955; }
QTextEdit { background:#151e27; border:1px solid #293845; selection-background-color:#257f7a; }
QSplitter::handle { background:#2a3945; width:5px; height:5px; }
QStatusBar { background:#18232d; color:#aab8c5; }
QToolTip { color:#e4ebf1; background:#263641; border:1px solid #4a5b68; }
)");
    MainWindow window;
    window.show();
    return app.exec();
}
