#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QScreen>
#include <QSaveFile>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QScreen *target = nullptr;
    for (auto *screen : app.screens())
        if (screen->name() == "Virtual-Sleep capture test") target = screen;
    if (!target) return 78;
    QWidget window;
    QVBoxLayout layout(&window);
    QLabel label("PRIVATE FRAME 0");
    label.setAlignment(Qt::AlignCenter);
    QLineEdit input;
    layout.addWidget(&label);
    layout.addWidget(&input);
    window.setStyleSheet("QWidget { background: #902020; color: white; font-size: 32px; }");
    QObject::connect(&input, &QLineEdit::returnPressed, [&input] {
        QFile file("/tmp/sunshine-private-input");
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            file.write(input.text().toUtf8());
            file.write("\n");
        }
        input.clear();
    });
    int sequence = 0;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        label.setText(QString("PRIVATE FRAME %1").arg(++sequence));
        QSaveFile tick("/tmp/sunshine-private-tick");
        if (tick.open(QIODevice::WriteOnly)) {
            tick.write(QByteArray::number(sequence));
            tick.commit();
        }
    });
    timer.start(500);
    window.winId();
    window.windowHandle()->setScreen(target);
    window.showFullScreen();
    window.activateWindow();
    input.setFocus();
    return app.exec();
}
