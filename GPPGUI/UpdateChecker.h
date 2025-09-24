#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QSystemTrayIcon>
#include <fstream>

class ElaText;

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(ElaText* statusText, QObject* parent = nullptr);
    void check();
    void downloadUpdate(const QString& url);
    bool getIsDownloadSucceed();

Q_SIGNALS:
    void closeWindowSignal();

private Q_SLOTS:
    void onReplyFinished(QNetworkReply* reply);
    void onReplyTimeout();
    void onDownloadFinished(QNetworkReply* reply);
    void onDownloadTimeout();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:

    bool hasNewVersion(std::string version1, std::string version2);

    bool m_downloadSuccess = false;
    ElaText* m_statusText = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;

    QNetworkAccessManager* m_checkManager = nullptr;
    QNetworkReply* m_checkReply = nullptr;
    QTimer* m_checkTimer = nullptr;
    QNetworkAccessManager* m_downloadManager = nullptr;
    QNetworkReply* m_downloadReply = nullptr;
    QTimer* m_downloadTimer = nullptr;

    const QString m_repoOwner = "julixian";
    const QString m_repoName = "GalTranslPP";
};

#endif // UPDATECHECKER_H
