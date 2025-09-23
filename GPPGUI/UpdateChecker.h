#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <string>
#include <QSystemTrayIcon>

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
    void onDownloadFinished(QNetworkReply* reply);

private:

    bool hasNewVersion(std::string version1, std::string version2);

    bool m_downloadSuccess = false;
    ElaText* m_statusText = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;

    QNetworkAccessManager* m_networkManager = nullptr;
    QNetworkAccessManager* m_downloadManager = nullptr;

    const QString m_repoOwner = "julixian";
    const QString m_repoName = "GalTranslPP";
};

#endif // UPDATECHECKER_H
