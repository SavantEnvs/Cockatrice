// Shared setup for the Cockatrice fuzz harnesses: one QCoreApplication per process (Qt needs it for
// QObject/event-loop based code) and a silent message handler so qDebug/qWarning output from parsers
// does not dominate the fuzzer's I/O. Nothing here changes per input.
#pragma once
#include <QCoreApplication>
#include <QtGlobal>

static void fuzzSilentHandler(QtMsgType, const QMessageLogContext &, const QString &) {}

static QCoreApplication *fuzzQtApp()
{
    static int argc = 1;
    static char arg0[] = "fuzz";
    static char *argv[] = {arg0, nullptr};
    static QCoreApplication *app = nullptr;
    if (!app) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qInstallMessageHandler(fuzzSilentHandler);
        app = new QCoreApplication(argc, argv);
    }
    return app;
}
