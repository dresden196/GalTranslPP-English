#include "UpdateChecker.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDesktopServices>

#include "ElaText.h"
#include "ElaMessageBar.h"

import Tool;
namespace fs = std::filesystem;

UpdateChecker::UpdateChecker(ElaText* statusText, QObject* parent) : QObject(parent), m_statusText(statusText)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/GPPGUI/Resource/Image/julixian_s.ico"));

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [=]()
        {
            Q_EMIT closeWindowSignal();
        });
}

void UpdateChecker::check()
{
    // GitHub API for the latest release
    QUrl url("https://api.github.com/repos/" + m_repoOwner + "/" + m_repoName + "/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_networkManager->get(request);
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, "更新检测失败", "网络连接失败，请检查网络设置。", 5000);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (!jsonDoc.isObject()) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, "更新检测失败", "获取更新信息失败。", 5000);
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    std::string latestVersion = jsonObj["tag_name"].toString().toStdString();

    if (hasNewVersion(latestVersion, GPPVERSION)) {
        ElaMessageBar::information(ElaMessageBarType::TopLeft, "检测到新版本", "最新版本: " + QString::fromStdString(latestVersion), 5000);
        if (!m_downloadSuccess && !m_downloadManager) {
            m_statusText->setText("检测到新版本！");
            QJsonArray assets = jsonObj["assets"].toArray();
            for (int i = 0; i < assets.size(); i++) {
                if (assets[i].toObject()["name"] == "GUICORE.zip") {
                    ElaMessageBar::information(ElaMessageBarType::TopLeft, "自动更新", "正在下载更新包...", 5000);
                    m_statusText->setText("下载更新...");
                    downloadUpdate(assets[i].toObject()["browser_download_url"].toString());
                    break;
                }
            }
        }
    }
    else {
        ElaMessageBar::success(ElaMessageBarType::TopLeft, "版本检测", "当前已是最新的版本", 5000);
    }
    reply->deleteLater();
}

void UpdateChecker::downloadUpdate(const QString& url)
{
    m_downloadManager = new QNetworkAccessManager(this);
    connect(m_downloadManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onDownloadFinished);

    QNetworkRequest request(url);
    m_downloadManager->get(request);
}

void UpdateChecker::onDownloadFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        ElaMessageBar::warning(ElaMessageBarType::TopLeft, "更新下载失败", "网络连接失败，请检查网络设置。", 5000);
        m_statusText->setText("更新下载失败");
        reply->deleteLater();
        m_downloadManager->deleteLater();
        m_downloadManager = nullptr;
        return;
    }

    std::ofstream ofs(L"GUICORE.zip", std::ios::binary);
    QByteArray responseData = reply->readAll();
    ofs.write(responseData.data(), responseData.size());
    ofs.close();

    ElaMessageBar::information(ElaMessageBarType::TopLeft, "更新下载成功", "将在程序关闭后自动安装更新", 5000);
    m_statusText->setText("更新下载成功");
    m_downloadSuccess = true;

    m_trayIcon->show();
    m_trayIcon->showMessage(
        "下载完成",                  // 标题
        "点击以关闭程序并安装更新",      // 内容
        QSystemTrayIcon::Information, // 图标类型 (Information, Warning, Critical)
        5000                          // 显示时长 (毫秒)
    );

    reply->deleteLater();
}

bool UpdateChecker::getIsDownloadSucceed()
{
    return m_downloadSuccess;
}

bool UpdateChecker::hasNewVersion(std::string latestVer, std::string currentVer)
{
    bool isCurrentVerPre = false;
    auto removePostfix = [&](std::string& v)
        {
            while (true) {
                if (v.ends_with("pre") || v.ends_with("Pre")) {
                    isCurrentVerPre = true;
                    v = v.substr(0, v.length() - 3);
                }
                else if (v.ends_with("p") || v.ends_with("P")) {
                    isCurrentVerPre = true;
                    v = v.substr(0, v.length() - 1);
                }
                else {
                    break;
                }
            }
        };
    
    removePostfix(latestVer);
    removePostfix(currentVer);
    std::string v1s = latestVer.find_last_of("v") == std::string::npos ? latestVer : latestVer.substr(latestVer.find_last_of("v") + 1);
    std::string v2s = currentVer.find_last_of("v") == std::string::npos ? currentVer : currentVer.substr(currentVer.find_last_of("v") + 1);

    std::vector<std::string> lastVerParts = splitString(v1s, '.');
    std::vector<std::string> currentVerParts = splitString(v2s, '.');

    size_t len = std::max(lastVerParts.size(), currentVerParts.size());

    for (size_t i = 0; i < len; i++) {
        int lastVerPart = i < lastVerParts.size() ? std::stoi(lastVerParts[i]) : 0;
        int currentVerPart = i < currentVerParts.size() ? std::stoi(currentVerParts[i]) : 0;

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
