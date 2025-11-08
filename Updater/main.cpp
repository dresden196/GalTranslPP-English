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
    if (hUser32) {
        uint64_t orgSetProcessDpiAwarenessContext = (uint64_t)(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
        if (orgSetProcessDpiAwarenessContext) {
            FnCast(orgSetProcessDpiAwarenessContext, SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
        FreeLibrary(hUser32);
    }
#endif

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("GalTransl++ Updater");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ {"p", "pid"}, "Process ID of the main application.", "pid" });
    parser.addOption({ {"s", "source"}, "Source path of the update package.", "source" });
    parser.addOption({ {"t", "target"}, "Target directory for installation.", "target" });
    parser.addOption({ {"r", "restart"}, "Restart the main application after update is installed.", "restart" });
    parser.addOption({ {"n", "newActionFlag"}, "Flag to indicate a new action.", "newActionFlag" });
    parser.addOption({ QStringList{"gppVersion"}, "Version of GalTranslPP.", "gppVersion" });
    parser.addOption({ QStringList{"pythonVersion"}, "Version of Python.", "pythonVersion" });
    parser.addOption({ QStringList{"promptVersion"}, "Version of Prompt.", "promptVersion" });
    parser.process(a);

    qint64 pid = parser.value("pid").toLongLong();
    QString sourceZip = parser.value("source");
    QString targetDir = parser.value("target");

    if (pid == 0 || sourceZip.isEmpty() || targetDir.isEmpty()) {
#ifdef Q_OS_WIN
        MessageBoxW(NULL, L"Invalid arguments provided.", L"GalTransl++ Updater", MB_ICONERROR | MB_TOPMOST);
#endif
        return -1;
    }

    if (parser.isSet("newActionFlag")) {
        try {
            std::string orgPythonVersion = parser.isSet("pythonVersion")? parser.value("pythonVersion").toStdString() : "1.0.0";
            std::string orgPromptVersion = parser.isSet("promptVersion")? parser.value("promptVersion").toStdString() : "1.0.0";
            std::set<std::string> excludePreFixes =
            {
                "BaseConfig/python-3.12.10-embed-amd64", "BaseConfig/pyScripts", "BaseConfig/Prompt.toml",
            };
            bool isCompatible = true;
            if (cmpVer(orgPythonVersion, PYTHONVERSION, isCompatible)) {
                excludePreFixes.erase("BaseConfig/python-3.12.10-embed-amd64");
                excludePreFixes.erase("BaseConfig/pyScripts");
            }
            if (cmpVer(orgPromptVersion, PROMPTVERSION, isCompatible)) {
#ifdef Q_OS_WIN
                int ret = MessageBoxW(NULL, L"检测到新版本的 Prompt，是否更新 Prompt (会覆盖当前的默认提示词)？", L"GalTransl++ Updater", MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
                if (ret == IDYES) {
                    excludePreFixes.erase("BaseConfig/Prompt.toml");
                }
#endif
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
            arguments << "--gppVersion" << QString::fromStdString(GPPVERSION);
            arguments << "--pythonVersion" << QString::fromStdString(PYTHONVERSION);
            arguments << "--promptVersion" << QString::fromStdString(PROMPTVERSION);
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
    MessageBoxW(NULL, L"GalTransl++ 更新成功", L"成功", MB_OK | MB_TOPMOST);
#endif

    if (parser.isSet("restart")) {
        QStringList args;
        args << "--pid" << QString::number(QApplication::applicationPid());
        QProcess::startDetached(parser.value("restart"), args, targetDir);
    }

    return 0;
}
