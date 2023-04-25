#include "qnasadownloader.h"

QNasaDownloader::QNasaDownloader(const QString &Login, const QString &Pass, QObject *parent): QObject(parent) {
    _requestIdx = 0;
    _login = Login;
    _pass = Pass;
    _nam.setCookieJar(&_cookieJar);
    _nam.setRedirectPolicy(QNetworkRequest::ManualRedirectPolicy);
}

QNasaDownloader::~QNasaDownloader() {

}

void QNasaDownloader::getFile(const QString &Url, const QString &OutputFilename) {
    _downloadUrl = Url;
    _outputFilename = OutputFilename;
    sendGetRequest(Url);
}

void QNasaDownloader::sendGetRequest(QString url) {
    _lastRequestData.clear();
    saveRequest(0, url, _lastRequestData);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    //request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    _reply = _nam.get(request);
    _reply->ignoreSslErrors();
    connect(_reply, SIGNAL(finished()), this, SLOT(slotReadyRead()));
    connect(_reply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
    connect(_reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(slotSslErrors(QList<QSslError>)));
}


void QNasaDownloader::sendPostRequest(QString url, const QString &data) {
    _lastRequestData.clear();
    _lastRequestData.append(data);
    saveRequest(0, url, _lastRequestData);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    //Content-Type: application/x-www-form-urlencoded
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    _reply = _nam.post(request, data.toUtf8());
    _reply->ignoreSslErrors();
    connect(_reply, SIGNAL(finished()), this, SLOT(slotReadyRead()));
    connect(_reply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
    connect(_reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(slotSslErrors(QList<QSslError>)));
}

void QNasaDownloader::slotReadyRead() {
    QNetworkReply *reply = (QNetworkReply *)sender();
    QString url = reply->url().toString();
    QByteArray data = reply->readAll();
    qDebug() << "QNasaDownloader::slotReadyRead" << data.size();
    QString sdata = QString(data);
    int httpResponceCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString new_url;
    saveRequest(httpResponceCode, url, sdata);
    if (httpResponceCode == 500) {
        // should be loggined try download once more
        sendGetRequest(_downloadUrl);
    } else if ((httpResponceCode >= 300) && (httpResponceCode < 400))  {
        new_url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
        sendGetRequest(new_url);
    } else {
        processData(httpResponceCode, url, data);
    }
}

void QNasaDownloader::slotError(QNetworkReply::NetworkError error) {
    qDebug() << "QNasaDownloader::slotError" << error << _reply->errorString();
    emit finished(_requestIdx < 50 ? _requestIdx * 2 : 102);
}

void QNasaDownloader::slotSslErrors(QList<QSslError> error_list) {
    qDebug() << "QNasaDownloader::slotSslErrors" << error_list;
    emit finished(_requestIdx < 50 ? _requestIdx * 2 + 1 : 103);
}

void QNasaDownloader::processData(int httpResponceCode, QString url, QByteArray &data) {
    qDebug() << "QNasaDownloader::processData" << data.length() << httpResponceCode << url << _downloadUrl;
    if (httpResponceCode != 200) {
        // something goes wrong, check logs
        emit finished(104);
        return;
    }
    QString SData = QString(data);
    if (SData.indexOf("You are already logged in") != -1) {
        // already loggined, need to find special link
        //sendGetRequest(_downloadUrl);
        redirectAfterLogin(SData);
        return;
    }

    if (url.indexOf("urs.earthdata.nasa.gov/oauth/authorize") != -1) {
        auth(url, SData);
        return;
    }

    if (url.indexOf(_downloadUrl) != -1) {
        storeFile(data);
        return;
    }

    // unknown url
    emit finished(105);
}

void QNasaDownloader::redirectAfterLogin(QString &Data) {
    int idx1 = Data.indexOf("id=\"redir_link\" href=\"");
    if (idx1 == -1) {
        emit finished(111);
        return;
    }
    idx1 += 22;
    int idx2 = Data.indexOf('"', idx1);
    if (idx2 == -1) {
        emit finished(112);
        return;
    }
    QString link = Data.mid(idx1, idx2 - idx1);
    sendGetRequest(link);
}

void QNasaDownloader::auth(QString &Url, QString &Data) {
    QString lFormUrl = Url, lFormData;
    if (!authPrepareForm(Data, lFormUrl, lFormData)) {
        // finished called in innner functions
        return;
    }

    sendPostRequest(lFormUrl, lFormData);
}

bool QNasaDownloader::authProcessUrl(QString &InData, QString &OutUrl, int FormIdx) {
    int idx1 = InData.indexOf("action=\"", FormIdx);
    if (idx1 == -1) {
        // wrong format
        emit finished(107);
        return false;
    }
    idx1 += 8;
    int idx2 = InData.indexOf('"', idx1);
    if (idx2 == -1) {
        // wrong format
        emit finished(108);
        return false;
    }
    QString action = InData.mid(idx1, idx2 - idx1);
    qDebug() << "QNasaDownloader::authProcessUrl" << "found action" << action;
    QString Protocol = OutUrl.mid(0, OutUrl.indexOf("://") + 3);
    QString UrlWithoutProtocol = OutUrl.mid(OutUrl.indexOf("://") + 3);
    QString FinalUrl;
    if (action.indexOf("/") == 0) {
        FinalUrl = Protocol + UrlWithoutProtocol.mid(0, UrlWithoutProtocol.indexOf("/")) + action;
    } else {
        FinalUrl = Protocol;
        if (UrlWithoutProtocol.indexOf("?") != -1) {
            FinalUrl.append(UrlWithoutProtocol.mid(0, UrlWithoutProtocol.indexOf("?")));
        } else {
            FinalUrl.append(UrlWithoutProtocol);
        }
        FinalUrl.append(action);
    }
    qDebug() << "QNasaDownloader::authProcessUrl" << OutUrl;
    qDebug() << "QNasaDownloader::authProcessUrl" << "Protocol" << Protocol;
    qDebug() << "QNasaDownloader::authProcessUrl" << "Witout Protocol" << UrlWithoutProtocol;
    qDebug() << "QNasaDownloader::authProcessUrl" << "Final" << FinalUrl;
    OutUrl = FinalUrl;
    return true;
}

QString QNasaDownloader::getProperty(const QString &Input, const QString &Property) {
    int idx1 = Input.indexOf(Property + "=\"");
    if (idx1 == -1) {
        return "";
    }
    idx1 += Property.length() + 2;
    int idx2 = Input.indexOf('"', idx1);
    if (idx2 == -1) {
        return "";
    }
    return Input.mid(idx1, idx2 - idx1);
}

void QNasaDownloader::authParseInput(const QString &Input, QString &Type, QString &Name, QString &Value) {
    qDebug() << "QNasaDownloader::authParseInput" << Input;
    Type = getProperty(Input, "type");
    Name = getProperty(Input, "name");
    Value = getProperty(Input, "value");
    qDebug() << "QNasaDownloader::authParseInput" << Type << Name << Value;
}

bool QNasaDownloader::authPrepareForm(QString &InData, QString &OutUrl, QString &OutForm) {
    qDebug() << "QNasaDownloader::authPrepareForm" << InData.length();
    int idx1, idx2, idx3, idx4;
    idx1 = InData.indexOf("<form id=\"login");
    if (idx1 == -1) {
        // wrong format
        emit finished(106);
        return false;
    }
    // parse form action to construct form recepient url
    if (!authProcessUrl(InData, OutUrl, idx1)) {
        return false;
    }

    idx2 = InData.indexOf("</form>", idx1);
    if (idx2 == -1) {
        // wrong format
        emit finished(109);
        return false;
    }
    idx3 = idx1;
    QString InputType, InputName, InputValue;
    OutForm.clear();
    while ((idx3 = InData.indexOf("<input", idx3)) != -1) {
        if (idx3 > idx2) {
            // login form finished
            break;
        }
        idx4 = InData.indexOf(">", idx3);
        authParseInput(InData.mid(idx3, idx4 - idx3), InputType, InputName, InputValue);
        if (InputType.isEmpty() || InputName.isEmpty()) {
            emit finished(110);
            return false;
        }
        if (!OutForm.isEmpty()) {
            OutForm.append("&");
        }
        if (InputName.compare("utf8") == 0) {
            OutForm.append("utf8=%E2%9C%93");
        } else if (InputName.compare("username") == 0) {
            OutForm.append(QString("username=%1").arg(QUrl::toPercentEncoding(_login)));
        } else if (InputName.compare("password") == 0) {
            OutForm.append(QString("password=%1").arg(QUrl::toPercentEncoding(_pass)));
        } else {
            OutForm.append(QString("%1=%2").arg(InputName, QUrl::toPercentEncoding(InputValue.replace(" ", "+"))));
        }
        idx3 = idx4;
    }
    return true;
}

void QNasaDownloader::storeFile(QByteArray &data) {
    qDebug() << "QNasaDownloader::storeFile" << data.length();
    QFile file(_outputFilename);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out.writeRawData(data, data.length());
    file.close();
    qDebug() << "file stored into " << _outputFilename;
    emit finished(0);
}

void QNasaDownloader::saveRequest(int httpResponceCode, QString url, QString &data) {
    qDebug() << "QNasaDownloader::saveRequest" << data.length() << httpResponceCode << url;
#ifdef QT_DEBUG
    QString file_name = QString("gnd_request_%2.html").arg(QString::number(++_requestIdx));
    QFile file(file_name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out << "<!-- " << QString::number(httpResponceCode) << " " << url << " " << _lastRequestData << " -->\n";
    out << data;
    file.close();
    qDebug() << "data stored into " << file_name;
#endif
}
