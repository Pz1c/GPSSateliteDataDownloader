#ifndef QNASADOWNLOADER_H
#define QNASADOWNLOADER_H

#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QList>
#include <QPair>
#include <QUuid>
#include <QDateTime>
#include <QLocale>
#include <QNetworkCookieJar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class QNasaDownloader : public QObject
{
    Q_OBJECT
public:
    explicit QNasaDownloader(const QString &Login, const QString &Pass, QObject *parent = 0);
    ~QNasaDownloader();


    void getFile(const QString &Url, const QString &OutputFilename);

signals:
    void finished(int ErrorCode);

protected:
    void processData(int httpResponceCode, QString url, QByteArray &data);
    void auth(QString &Url, QString &Data);
    bool authPrepareForm(QString &InData, QString &OutUrl, QString &OutForm);
    bool authProcessUrl(QString &InData, QString &OutUrl, int FormIdx);
    void authParseInput(const QString &Input, QString &Type, QString &Name, QString &Value);
    void redirectAfterLogin(QString &Data);

    void storeFile(QByteArray &data);

private slots:

    void slotReadyRead();
    void slotError(QNetworkReply::NetworkError error);
    void slotSslErrors(QList<QSslError> error_list);

private:
    void saveRequest(int httpResponceCode, QString url, QString &data);
    void sendGetRequest(QString url);
    void sendPostRequest(QString url, const QString &data);
    QString getProperty(const QString &Input, const QString &Property);

private:
    // bussines login
    QString _downloadUrl;
    QString _login;
    QString _pass;
    QString _outputFilename;

    // network
    QNetworkAccessManager _nam;
    QNetworkReply *_reply;
    QNetworkCookieJar _cookieJar;
    int _requestIdx;
    QString _lastRequestData;

    
};
#endif // QNASADOWNLOADER_H
