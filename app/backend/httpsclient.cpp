#include "httpsclient.h"

#include <QSslSocket>
#include <QDebug>
#include <QSslCertificate>
#include <QCoreApplication>

HttpsClient::HttpsClient(QString address,int port) {
    socket = new QSslSocket(this);
    connect(socket, &QSslSocket::encrypted, this, &HttpsClient::onEncrypted);
    // connect(socket, static_cast<void (QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),
    //         this, &HttpsClient::sslError);
    socket->connectToHostEncrypted(address, port); // 使用你的目标服务器地址和端口
}

void HttpsClient::onEncrypted() {
    qDebug() << "Connection encrypted.";
    QSslCertificate cert = socket->peerCertificate();
    qDebug() << "Certificate subject:" << cert.subjectInfo(QSslCertificate::CommonName);
    qDebug() << "Certificate issuer:" << cert.issuerInfo(QSslCertificate::Organization);
}

void HttpsClient::sslError(QAbstractSocket::SocketError socketError) {
    qDebug() << "SSL error:" << socket->errorString();
}
