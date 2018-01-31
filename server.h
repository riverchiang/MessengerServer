#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QtNetwork>

#include <QDebug>

class QLabel;
class QTcpServer;
class QNetworkSession;

namespace Ui {
class Server;
}

struct clientInfo
{
    QString name;
    QString passwd;
    quint64 uid;
};

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();

private slots:
    void sessionOpened();
    void sendTimeStamp();
    void readData();

private:
    Ui::Server *ui;
    QLabel *statusLabel = nullptr;
    QTcpServer *tcpServer = nullptr;
    QNetworkSession *networkSession = nullptr;
    QDataStream in;
    quint64 cmdID = 0;
    quint64 blockSize = 0;
    quint64 uid = 0;
    QVector<struct clientInfo> clientVector;

    void sendReturn(QTcpSocket *socket, quint64 id, QString message);
};

#endif // SERVER_H
