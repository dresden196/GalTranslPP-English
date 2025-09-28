#include "UpdateChecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include <QCryptographicHash>

#include "ElaText.h"
#include "ElaMessageBar.h"

import Tool;
namespace fs = std::filesystem;

// 计算文件的 SHA-256 哈希值
QString calculateFileSha256(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString(); // 返回空字符串表示失败
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    const qint64 bufferSize = 8192;
    while (!file.atEnd()) {
        // 从文件中读取一块数据，然后添加到哈希计算中
        QByteArray buffer = file.read(bufferSize);
        hash.addData(buffer);
    }

    file.close();
    return hash.result().toHex();
}

UpdateChecker::UpdateChecker(toml::table& globalConfig, ElaText* statusText, QObject* parent) : 
    QObject(parent), m_statusText(statusText), m_globalConfig(globalConfig)
{
    m_checkManager = new QNetworkAccessManager(this);
    connect(m_checkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);

    m_downloadManager = new QNetworkAccessManager(this);
    connect(m_downloadManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onDownloadFinished);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/GPPGUI/Resource/Image/julixian_s.ico"));

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [=]()
        {
            Q_EMIT closeWindowSignal();
        });

    m_checkTimer = new QTimer(this);
    m_checkTimer->setSingleShot(true);
    connect(m_checkTimer, &QTimer::timeout, this, &UpdateChecker::onReplyTimeout);

    m_downloadTimer = new QTimer(this);
    m_downloadTimer->setSingleShot(true);
    connect(m_downloadTimer, &QTimer::timeout, this, &UpdateChecker::onDownloadTimeout);
}

void UpdateChecker::check(bool forDownload)
{
    // GitHub API for the latest release
    QUrl url("https://api.github.com/repos/" + m_repoOwner + "/" + m_repoName + "/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_checkReply = m_checkManager->get(request);
    m_isForDownloadMap[m_checkReply] = forDownload;
    m_checkTimer->start(10000);
}

void UpdateChecker::onReplyTimeout()
{
    if (m_checkReply && m_checkReply->isRunning()) {
        m_checkReply->abort();
    }
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply)
{
    m_checkTimer->stop();
    bool forDownload = m_isForDownloadMap[reply];
    m_isForDownloadMap.erase(m_isForDownloadMap.find(reply));
    m_checkReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("更新检测失败"), tr("网络连接失败，请检查网络设置。"), 5000);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (!jsonDoc.isObject()) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("更新检测失败"), tr("获取更新信息失败。"), 5000);
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    std::string latestVersion = jsonObj["tag_name"].toString().toStdString();
    bool canUpdate = forDownload ? true : m_globalConfig["autoDownloadUpdate"].value_or(true);
    bool isCompatible = true;
    bool hasNewVersion = cmpVer(latestVersion, GPPVERSION, isCompatible);

    if (hasNewVersion) {
        if (!forDownload) {
            ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("检测到新版本"), tr("最新版本: ") + QString::fromStdString(latestVersion), 5000);
        }
        if (canUpdate && !m_downloadReply) {
            if (!forDownload && !isCompatible) {
                ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("不兼容更新"), tr("最新版含有不兼容当前版本的内容，请在确认 github 发布页更新日志后再酌情下载。"), 5000);
            }
            else {
                bool hasUpdateFile = false;
                if (fs::exists(L"GUICORE.zip")) {
                    QString currentFileHash = "sha256:" + calculateFileSha256("GUICORE.zip");
                    QJsonArray assets = jsonObj["assets"].toArray();
                    for (int i = 0; i < assets.size(); i++) {
                        if (assets[i].toObject()["name"] != "GUICORE.zip") {
                            continue;
                        }
                        QString updateFileHash = assets[i].toObject()["digest"].toString();
                        if (updateFileHash == currentFileHash) {
                            m_statusText->setText(tr("更新下载已完成"));
                            m_downloadSuccess = true;
                            hasUpdateFile = true;
                            m_trayIcon->showMessage(
                                tr("下载完成"),                  // 标题
                                    tr("点击以关闭程序并安装更新"),      // 内容
                                QSystemTrayIcon::Information, // 图标类型 (Information, Warning, Critical)
                                5000                          // 显示时长 (毫秒)
                            );
                        }
                        break;
                    }
                }
                if (!hasUpdateFile) {
                    m_statusText->setText(tr("检测到新版本！"));
                    QJsonArray assets = jsonObj["assets"].toArray();
                    for (int i = 0; i < assets.size(); i++) {
                        if (assets[i].toObject()["name"] != "GUICORE.zip") {
                            continue;
                        }
                        ElaMessageBar::information(ElaMessageBarType::TopLeft, tr("下载更新"), tr("正在下载更新包..."), 5000);
                        m_statusText->setText(tr("下载更新..."));
                        downloadUpdate(assets[i].toObject()["browser_download_url"].toString());
                        break;
                    }
                }
            }
        }
    }
    else {
        ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("版本检测"), tr("当前已是最新的版本"), 5000);
    }
    reply->deleteLater();
    Q_EMIT checkCompleteSignal(hasNewVersion);
}

void UpdateChecker::downloadUpdate(const QString& url)
{
    QNetworkRequest request(url);
    m_downloadReply = m_downloadManager->get(request);

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateChecker::onDownloadProgress);

    m_downloadTimer->start(15000);
}

void UpdateChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_statusText->setText(QString::fromStdString(
        std::format("下载更新... {:.1f}/{:.1f} MB", (double)bytesReceived / 1024 / 1024, (double)bytesTotal / 1024 / 1024)
    ));
    m_downloadTimer->start(15000);
}

void UpdateChecker::onDownloadTimeout()
{
    if (m_downloadReply && m_downloadReply->isRunning()) {
        m_downloadReply->abort();
    }
}

void UpdateChecker::onDownloadFinished(QNetworkReply* reply)
{
    m_downloadTimer->stop();
    m_downloadReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("更新下载失败"), tr("网络连接失败，请检查网络设置。"), 5000);
        m_statusText->setText(tr("更新下载失败"));
        reply->deleteLater();
        return;
    }

    std::ofstream ofs(L"GUICORE.zip", std::ios::binary);
    QByteArray responseData = reply->readAll();
    ofs.write(responseData.data(), responseData.size());
    ofs.close();

    ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("更新下载成功"), tr("将在程序关闭后自动安装更新"), 5000);
    m_statusText->setText(tr("更新下载成功"));
    m_downloadSuccess = true;

    // 其实我有点想用 wintoast ...，QT这个taryIcon真有点丑
    m_trayIcon->showMessage(
        tr("下载完成"),                  // 标题
            tr("点击以关闭程序并安装更新"),      // 内容
        QSystemTrayIcon::Information, // 图标类型 (Information, Warning, Critical)
        5000                          // 显示时长 (毫秒)
    );

    reply->deleteLater();
}

bool UpdateChecker::getIsDownloadSucceed()
{
    return m_downloadSuccess;
}

bool UpdateChecker::getIsDownloading()
{
    return m_downloadReply != nullptr;
}

bool UpdateChecker::cmpVer(std::string latestVer, std::string currentVer, bool& isCompatible)
{
    bool isCurrentVerPre = false;
    auto removePostfix = [&](std::string& v)
        {
            while (true) {
                if (
                    !v.empty() && 
                    (v.back() < '0' || v.back() > '9')
                    ) {
                    v.pop_back();
                    isCurrentVerPre = true;
                }
                else {
                    break;
                }
            }
        };
    
    removePostfix(currentVer);
    std::string v1s = latestVer.find_last_of("v") == std::string::npos ? latestVer : latestVer.substr(latestVer.find_last_of("v") + 1);
    std::string v2s = currentVer.find_last_of("v") == std::string::npos ? currentVer : currentVer.substr(currentVer.find_last_of("v") + 1);

    std::vector<std::string> lastVerParts = splitString(v1s, '.');
    std::vector<std::string> currentVerParts = splitString(v2s, '.');

    size_t len = std::max(lastVerParts.size(), currentVerParts.size());

    for (size_t i = 0; i < len; i++) {
        int lastVerPart = i < lastVerParts.size() ? std::stoi(lastVerParts[i]) : 0;
        int currentVerPart = i < currentVerParts.size() ? std::stoi(currentVerParts[i]) : 0;
        if (i == 0 && lastVerPart > currentVerPart) {
            isCompatible = false;
        }

        if (lastVerPart > currentVerPart) {
            return true;
        }
        else if (lastVerPart < currentVerPart) {
            return false;
        }
    }

    if (isCurrentVerPre) {
        return true;
    }

    return false;
}
