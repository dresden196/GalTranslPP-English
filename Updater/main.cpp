#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QThread> // 用于延时
#pragma comment(lib, "../lib/GalTranslPP.lib")


#ifdef Q_OS_WIN
#include <windows.h>
#pragma comment(lib, "User32.lib")
#endif

import Tool;

template<typename FnCastTo>
FnCastTo FnCast(uint64_t fnToCast, FnCastTo) {
    return (FnCastTo)fnToCast;
}

void waitForProcessToExit(qint64 pid) {
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (hProcess != NULL) {
        WaitForSingleObject(hProcess, INFINITE);
        CloseHandle(hProcess);
    }
#else

#endif
    qDebug() << "Main application (PID:" << pid << ") has exited.";
}


int main(int argc, char* argv[]) {

#ifdef Q_OS_WIN
    HMODULE hUser32 = LoadLibraryW(L"User32.dll");
    uint64_t orgSetProcessDpiAwarenessContext = (uint64_t)(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
    if (orgSetProcessDpiAwarenessContext) {
        FnCast(orgSetProcessDpiAwarenessContext, SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
#endif

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("Updater");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ {"p", "pid"}, "Process ID of the main application.", "pid" });
    parser.addOption({ {"s", "source"}, "Source path of the update package.", "source" });
    parser.addOption({ {"t", "target"}, "Target directory for installation.", "target" });
    parser.addOption({ {"r", "restart"}, "Restart the main application after update is installed.", "restart" });
    parser.process(a);

    qint64 pid = parser.value("pid").toLongLong();
    QString sourceZip = parser.value("source");
    QString targetDir = parser.value("target");
    bool restart = parser.isSet("restart");

    if (pid == 0 || sourceZip.isEmpty() || targetDir.isEmpty()) {
        qWarning() << "Invalid arguments provided.";
#ifdef Q_OS_WIN
        MessageBoxW(NULL, L"Invalid arguments provided.", L"Updater", MB_ICONERROR);
#endif
        return -1;
    }

    qDebug() << "Updater started. Waiting for main application to close...";

    // 1. 等待主程序退出
    waitForProcessToExit(pid);

    // 稍微延时，确保文件锁已完全释放
    QThread::msleep(1000);

    qDebug() << "Unzipping" << sourceZip << "to" << targetDir;

    // 2. 解压并覆盖文件
    try {
        extractZip(ascii2Wide(sourceZip.toStdString()), ascii2Wide(targetDir.toStdString()));
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to extract update package:" << e.what();
#ifdef Q_OS_WIN
        MessageBoxW(NULL, L"Failed to extract update package.", L"Updater", MB_ICONERROR);
#endif
        return -1;
    }

    qDebug() << "Update extracted successfully.";

    // 3. 清理工作
    QFile::remove(sourceZip);
    qDebug() << "Cleaned up temporary files.";

#ifdef Q_OS_WIN
    MessageBoxW(NULL, L"更新成功", L"成功", MB_OK);
#endif
    if (restart) {
        qDebug() << "Restarting main application...";
        QProcess::startDetached("GalTranslPP_GUI.exe", QStringList(), targetDir);
        qDebug() << "Main application restarted.";
    }

    return 0;
}
