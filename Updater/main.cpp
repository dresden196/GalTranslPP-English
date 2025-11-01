#include <toml.hpp>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QApplication>
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
namespace fs = std::filesystem;

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
}


int main(int argc, char* argv[]) {

#ifdef Q_OS_WIN
    HMODULE hUser32 = LoadLibraryW(L"User32.dll");
    uint64_t orgSetProcessDpiAwarenessContext = (uint64_t)(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
    if (orgSetProcessDpiAwarenessContext) {
        FnCast(orgSetProcessDpiAwarenessContext, SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
    FreeLibrary(hUser32);
#endif

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("Updater");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ {"p", "pid"}, "Process ID of the main application.", "pid" });
    parser.addOption({ {"s", "source"}, "Source path of the update package.", "source" });
    parser.addOption({ {"t", "target"}, "Target directory for installation.", "target" });
    parser.addOption({ {"r", "restart"}, "Restart the main application after update is installed.", "restart" });
    parser.addOption({ {"n", "newActionFlag"}, "Flag to indicate a new action.", "newActionFlag" });
    parser.process(a);

    qint64 pid = parser.value("pid").toLongLong();
    QString sourceZip = parser.value("source");
    QString targetDir = parser.value("target");

    if (pid == 0 || sourceZip.isEmpty() || targetDir.isEmpty()) {
#ifdef Q_OS_WIN
        MessageBoxW(NULL, L"Invalid arguments provided.", L"Updater", MB_ICONERROR | MB_TOPMOST);
#endif
        return -1;
    }

    if (parser.isSet("newActionFlag")) {
        try {
            extractFileFromZip(sourceZip.toStdWString(), targetDir.toStdWString(), "update.toml");
            const auto updateConfig = toml::parse(fs::path(L"update.toml"));
            fs::remove(L"update.toml");
            std::set<std::string> excludePreFixes =
            {
                "BaseConfig/python-3.12.10-embed-amd64", "BaseConfig/pyScripts", "update.toml"
            };
            bool isCompatible = true;
            if (cmpVer(toml::find<std::string>(updateConfig, "PYTHON", "version"), PYTHONVERSTION, isCompatible)) {
                excludePreFixes.erase("BaseConfig/python-3.12.10-embed-amd64");
                excludePreFixes.erase("BaseConfig/pyScripts");
            }
            extractZipExclude(sourceZip.toStdWString(), targetDir.toStdWString(), excludePreFixes);
        }
        catch (const std::exception&) {
#ifdef Q_OS_WIN
            MessageBoxW(NULL, L"Failed to extract update package.", L"Updater", MB_ICONERROR | MB_TOPMOST);
#endif
            return -1;
        }
        return 0;
    }

    // 1. 等待主程序退出
    waitForProcessToExit(pid);

    // 稍微延时，确保文件锁已完全释放
    QThread::msleep(1000);

    // 2. 解压并覆盖文件
    try {
        extractFileFromZip(sourceZip.toStdWString(), targetDir.toStdWString(), "Updater_new.exe");
        if (fs::exists(L"Updater_new.exe")) {
            QStringList arguments;
            arguments << "--newActionFlag" << QString::number(QApplication::applicationPid());
            arguments << "--pid" << QString::number(QApplication::applicationPid());
            arguments << "--source" << sourceZip << "--target" << targetDir;
            QProcess::execute("Updater_new.exe", arguments);
        }
    }
    catch (const std::exception&) {
#ifdef Q_OS_WIN
        MessageBoxW(NULL, L"Failed to extract updater_new.exe.", L"Updater", MB_ICONERROR | MB_TOPMOST);
#endif
        return -1;
    }

    QFile::remove(sourceZip);

#ifdef Q_OS_WIN
    MessageBoxW(NULL, L"更新成功", L"成功", MB_OK | MB_TOPMOST);
#endif

    if (parser.isSet("restart")) {
        QStringList args;
        args << "--pid" << QString::number(QApplication::applicationPid());
        QProcess::startDetached(parser.value("restart"), args, targetDir);
    }

    return 0;
}
