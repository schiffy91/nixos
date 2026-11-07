#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <cstdio>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const auto screens = app.screens();
    const bool hotplug = app.arguments().contains("--allow-hotplug");
    if (screens.size() != 2 && !hotplug) {
        std::fprintf(stderr, "Expected two independent screens, got %lld\n",
                     static_cast<long long>(screens.size()));
        return 78;
    }
    const auto showScreen = [&app](QScreen *screen) {
        const auto index = app.screens().indexOf(screen);
        auto *window = new QWidget;
        window->setWindowTitle(QStringLiteral("Sleep capture target: ") + screen->name());
        auto *layout = new QVBoxLayout(window);
        auto *label = new QLabel(QString("PRIVATE DISPLAY %1").arg(index));
        label->setAlignment(Qt::AlignCenter);
        auto *input = new QLineEdit;
        layout->addWidget(label);
        layout->addWidget(input);
        window->setStyleSheet(index == 0
            ? "QWidget { background: #902020; color: white; font-size: 30px; }"
            : "QWidget { background: #206090; color: white; font-size: 30px; }");
        QObject::connect(input, &QLineEdit::returnPressed, [input] {
            const QString value = input->text();
            QFile file("/tmp/multioutput-input");
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                file.write(value.toUtf8());
                file.write("\n");
            }
            input->clear();
            if (value == "SLEEP")
                QProcess::startDetached("systemctl", {"suspend"});
        });
        window->winId();
        window->windowHandle()->setScreen(screen);
        window->showFullScreen();
        input->setFocus();
    };
    // Show the first screen last, giving its real Wayland input field focus.
    for (auto screen = screens.crbegin(); screen != screens.crend(); ++screen)
        showScreen(*screen);
    if (hotplug)
        QObject::connect(&app, &QGuiApplication::screenAdded, &app, showScreen);
    return app.exec();
}
