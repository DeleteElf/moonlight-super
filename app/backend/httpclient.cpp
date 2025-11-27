#include "httpclient.h"

#include <QNetworkProxy>
#include <QEventLoop>
#include <QTimer>

#include <QCoreApplication>

HttpClient::HttpClient(QObject *parent)
    : QObject{parent}
{

}

QNetworkRequest HttpClient::createRequest(QString url){
    QUrl requestUrl(url);
    return createRequest(requestUrl);
}

QNetworkRequest HttpClient::createRequest(QUrl url){
    QNetworkRequest request(url);
    if(url.scheme()=="https"){
        QSslConfiguration config ;
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
        config.setProtocol(QSsl::TlsV1_2);
        request.setSslConfiguration(config);
    }
    return request;
}

void HttpClient::setHeader(QNetworkRequest request,const QByteArray name,const QByteArray value){
     request.setRawHeader(name,value);
}

/**
 * @brief HttpClient::get
 * @param computer
 * @param url
 * @param username
 * @param pwd
 * @return
 */
QNetworkReply* HttpClient::get(QNetworkRequest request){
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    manager.setProxy(noProxy);
    return manager.get(request);
}

QNetworkReply* HttpClient::post(QNetworkRequest request,QByteArray body){
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    manager.setProxy(noProxy);
    return manager.post(request,body);
}

void HttpClient::requestTo(QNetworkReply* reply,int timeoutMs){
    if(timeoutMs<0){
        timeoutMs=REQUEST_TIMEOUT_MS;
    }
    // Run the request with a timeout if requested
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &loop, &QEventLoop::quit);
    if (timeoutMs) {
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    }

    qDebug() << "Executing request:" << reply->request().url();

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    // Abort the request if it timed out
    if (!reply->isFinished())
    {
        qWarning() << "Aborting timed out request for" << reply->request().url();
        reply->abort();
    }

    manager.clearAccessCache();

    // Handle error
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << reply->request().url() << "request failed with error:" << reply->error();
        if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
            // This will trigger falling back to HTTP for the serverinfo query
            // then pairing again to get the updated certificate.
            HttpResponseException exception(401, "Server certificate mismatch");
            delete reply;
            throw exception;
        }
        else if (reply->error() == QNetworkReply::OperationCanceledError) {
            HttpResponseException exception(QNetworkReply::OperationCanceledError, "Request rejected！");
            delete reply;
            throw exception;
        }
        else {
            HttpResponseException exception(reply->error(), reply->errorString());
            delete reply;
            throw exception;
        }
    }
}

QString HttpClient::handleResponseString(QNetworkReply* reply){
    QString ret;
    QTextStream stream(reply);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    ret = stream.readAll();
    qDebug() << "收到的应答：" << ret ;
    return ret;
}
