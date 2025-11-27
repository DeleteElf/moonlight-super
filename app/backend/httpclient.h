#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define REQUEST_TIMEOUT_MS 5000

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    QNetworkRequest createRequest(QString url);
    QNetworkRequest createRequest(QUrl url);
    void setHeader(QNetworkRequest request,const QByteArray name,const QByteArray value);
    QNetworkReply* get(QNetworkRequest request);
    QNetworkReply* post(QNetworkRequest request,QByteArray body);
    void requestTo(QNetworkReply* reply,int timeoutMs=-1);
    QString handleResponseString(QNetworkReply* reply);
signals:

private:
    QNetworkAccessManager manager;
};



class HttpResponseException : public std::exception
{
public:
    HttpResponseException(int statusCode, QString message) :
        m_StatusCode(statusCode),
        m_StatusMessage(message)
    {

    }

    const char* what() const throw()
    {
        return m_StatusMessage.toLatin1();
    }

    const char* getStatusMessage() const
    {
        return m_StatusMessage.toLatin1();
    }

    int getStatusCode() const
    {
        return m_StatusCode;
    }

    QString toQString() const
    {
        return m_StatusMessage + " (Error " + QString::number(m_StatusCode) + ")";
    }

private:
    int m_StatusCode;
    QString m_StatusMessage;
};


#endif // HTTPCLIENT_H
