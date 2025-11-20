#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QTextStream>

#ifndef QT_DEBUG    // <-- Only compile this in Release mode
void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream out(stdout);
    switch (type) {
    case QtDebugMsg:
        out << "[DEBUG] " << msg << "\n";
        break;
    case QtWarningMsg:
        out << "[WARNING] " << msg << "\n";
        break;
    case QtCriticalMsg:
        out << "[CRITICAL] " << msg << "\n";
        break;
    case QtFatalMsg:
        out << "[FATAL] " << msg << "\n";
        abort();
    }
    out.flush();
}
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifndef QT_DEBUG
    // Use custom logging only in Release mode
    qInstallMessageHandler(myMessageHandler);
#endif

    MainWindow w;
    w.show();
    //w.Test1();
    //w.RunVerity();
    return a.exec();
}
