#include "UpdateChecker.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDesktopServices>

#include "ElaMessageBar.h"

import Tool;

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);
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
        ElaMessageBar::information(ElaMessageBarType::TopLeft, "检测到新版本", "最新版本: " + QString::fromStdString(GPPVERSION), 5000);
    }
    else {
        ElaMessageBar::success(ElaMessageBarType::TopLeft, "版本检测", "当前已是最新的版本", 5000);
    }
    reply->deleteLater();
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
