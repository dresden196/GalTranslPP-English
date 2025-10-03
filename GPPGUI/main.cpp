#include <QDir>
#include <QApplication>
#include <QTranslator>
#include <QCoreApplication>
#include <QThread>
#include <QCommandLineParser>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <toml.hpp>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include "ElaApplication.h"
#include "mainwindow.h"

#pragma comment(lib, "../lib/GalTranslPP.lib")
#pragma comment(lib, "../lib/ElaWidgetTools.lib")

import std;
namespace fs = std::filesystem;

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

int main(int argc, char* argv[])
{

#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    QApplication a(argc, argv);
    QDir::setCurrent(QApplication::applicationDirPath());

    try {
        bool checkUpdate = true;
        QTranslator translator;
        QTranslator qtBaseTranslator; // 用于翻译 Qt 内置对话框，如 QMessageBox 的按钮
        try {
            toml::value config = toml::parse(fs::path(L"BaseConfig/globalConfig.toml"));
            checkUpdate = toml::get_or(config["autoCheckUpdate"], true);
            std::string language = toml::get_or(config["language"], "zh_CN");
            if (language == "en") {
                if (qtBaseTranslator.load("qt_en.qm", "translations")) {
                    a.installTranslator(&qtBaseTranslator);
                }
                if (translator.load("Translation_en.qm", "translations")) {
                    a.installTranslator(&translator);
                }
            }
        }
        catch (...) {

        }

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addOption({ {"p", "pid"}, "Process ID of the updater application.", "pid" });
        parser.process(a);
        if (parser.isSet("pid")) {
            qint64 pid = parser.value("pid").toLongLong();
            waitForProcessToExit(pid);
        }

        if (fs::exists(L"Updater_new.exe")) {
            try {
                fs::rename(L"Updater_new.exe", L"Updater.exe");
            }
            catch (const fs::filesystem_error& e) {
#ifdef Q_OS_WIN
                MessageBoxW(nullptr, QString(e.what()).toStdWString().c_str(), L"Updater 更新错误", MB_ICONERROR);
#endif
            }
        }

        // 1. 使用 QSharedMemory 防止多实例运行
        // 使用一个唯一的key
        const QString uniqueKey = "{957C27D8-37BB-4F54-9EBE-D0F5C701CBBF}";
        QSharedMemory sharedMemory(uniqueKey);

        // 尝试附加到共享内存。
        // 如果成功，说明已有实例在运行。
        if (sharedMemory.attach()) {
            // --- 这是第二个实例（客户端）的逻辑 ---
            // 创建一个本地套接字，用于和第一个实例通信
            QLocalSocket socket;
            socket.connectToServer(uniqueKey); // 使用相同的key作为服务器名
            // 等待连接成功（最多等待500毫秒）
            if (socket.waitForConnected(500)) {
                // 发送一个简单的消息，告诉服务器激活窗口
                socket.write("activate");
                socket.waitForBytesWritten(500);
                socket.disconnectFromServer();
            }
            // 客户端使命完成，退出
            return 0;
        }


        // --- 如果程序运行到这里，说明这是第一个实例（服务器） ---
        // 尝试创建共享内存段
        if (!sharedMemory.create(1)) {
#ifdef Q_OS_WIN
            MessageBoxW(nullptr, L"无法创建共享内存段，程序即将退出。", L"错误", MB_ICONERROR);
#endif
            return 1; // 创建失败，退出 
        }

        eApp->init();
        MainWindow w;

        // 2. 创建 QLocalServer，用于接收来自新实例的消息
        QLocalServer server;

        // 当有新连接时，触发 newConnection 信号
        QObject::connect(&server, &QLocalServer::newConnection, [&]() {
            // 获取连接
            QLocalSocket* clientConnection = server.nextPendingConnection();
            QObject::connect(clientConnection, &QLocalSocket::disconnected,
                clientConnection, &QLocalSocket::deleteLater);

            // 当接收到数据时
            QObject::connect(clientConnection, &QLocalSocket::readyRead, [&]() {
                QByteArray data = clientConnection->readAll();
                if (data == "activate") {
                    if (w.isMinimized()) {
                        w.setWindowState(w.windowState() & ~Qt::WindowMinimized);
                    }
                    Qt::WindowFlags flags = w.windowFlags();
                    w.setWindowFlags(flags | Qt::WindowStaysOnTopHint);
                    w.show();
                    w.activateWindow();
                    w.moveToCenter();
                    w.setWindowFlags(flags);
                    w.show();
                }
                });
            });

        // 开始监听。如果监听失败，可能是之前的实例崩溃但没释放server name。
        // 我们需要确保在程序退出时正确关闭服务器。
        if (!server.listen(uniqueKey)) {
            // 如果监听失败，可能是因为上次程序异常退出导致 server name 未被释放
            // 尝试移除旧的 server name
            QLocalServer::removeServer(uniqueKey);
            // 再次尝试监听
            if (!server.listen(uniqueKey)) {
#ifdef Q_OS_WIN
                MessageBoxW(nullptr, L"无法启动本地服务，程序即将退出。", L"错误", MB_ICONERROR);
#endif
                return 1;
            }
        }

        w.show();
        if (checkUpdate) {
            w.checkUpdate();
        }
        int result = a.exec();

        // 程序退出前，确保服务器关闭
        server.close();
        try {
            fs::remove_all(L"cache");
        }
        catch (const fs::filesystem_error& e) {
#ifdef Q_OS_WIN
            MessageBoxW(nullptr, QString(e.what()).toStdWString().c_str(), L"缓存删除错误", MB_ICONERROR);
#endif
        }
        return result;
    }
    catch (const toml::exception& e) {
#ifdef Q_OS_WIN
        MessageBoxW(nullptr, QString::fromStdString(e.what()).toStdWString().c_str(), L"TOML 错误", MB_ICONERROR);
#endif
        return 1;
    }
    catch (const std::exception& e) {
#ifdef Q_OS_WIN
        MessageBoxW(nullptr, QString::fromStdString(e.what()).toStdWString().c_str(), L"标准错误", MB_ICONERROR);
#endif
        return 1;
    }
    catch (...) {
#ifdef Q_OS_WIN
        MessageBoxW(nullptr, L"遇到了未知的错误，程序即将退出。", L"错误", MB_ICONERROR);
#endif
        return 1;
    }
    
    return 0;
}
