#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QSystemTrayIcon>
#include <fstream>
#include <toml.hpp>

class ElaText;

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(toml::ordered_value& globalConfig, ElaText* statusText, QObject* parent = nullptr);
    void check(bool forDownload = false);
    void downloadUpdate(const QString& url);
    bool getIsDownloadSucceed();
    bool getIsDownloading();

Q_SIGNALS:
    void checkCompleteSignal(bool hasNewVersion);
    void applyUpdateAndRestartSignal();

private Q_SLOTS:
    void onReplyFinished(QNetworkReply* reply);
    void onReplyTimeout();
    void onDownloadFinished(QNetworkReply* reply);
    void onDownloadTimeout();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:

    bool cmpVer(const std::string& latestVer, const std::string& currentVer, bool& isCompatible);

    bool m_downloadSuccess = false;
    ElaText* m_statusText = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;

    QNetworkAccessManager* m_checkManager = nullptr;
    QNetworkReply* m_checkReply = nullptr;
    QTimer* m_checkTimer = nullptr;
    QNetworkAccessManager* m_downloadManager = nullptr;
    QNetworkReply* m_downloadReply = nullptr;
    QTimer* m_downloadTimer = nullptr;
    QMap<QNetworkReply*, bool> m_isForDownloadMap;

    toml::ordered_value& m_globalConfig;
    const QString m_repoOwner = "julixian";
    const QString m_repoName = "GalTranslPP";
};

#endif // UPDATECHECKER_H
