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

struct message
{
    int uid;
    QString text;
};

struct clientInfo
{
    QString name;
    QString passwd;
    int uid;
    bool hasClientIcon;
    QVector<struct message> messageBox;
};

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();

private slots:
    void sessionOpened();
    void handleNewConn();
    void readData();

private:
    Ui::Server *ui;
    enum MessengerCmd{None = 0, Register, Login, FriendList, TalkSend, TalkRecv, PicSend, PicMeta, PicRecv};
    QLabel *statusLabel = nullptr;
    QTcpServer *tcpServer = nullptr;
    QNetworkSession *networkSession = nullptr;
    QDataStream in;
    quint64 cmdID = None;
    quint64 blockSize = 0;
    quint64 clientUid = 0;
    int uid = 1;
    QVector<struct clientInfo> clientVector;

    QString picFolder = "C:/Users/A60013/Pictures/temp/server/";
    //QString picFolder = "L:/Users/admin/Pictures/temp/server/";
    void sendReturn(QTcpSocket *socket, quint64 id, QString message);
    void sendNetworkfile(QString filePath, QTcpSocket *socket, int uid);

    // Recv network function
    void recvClientCmdRegister(QString recvData, QTcpSocket *socket);
    void recvClientCmdLogin(QString recvData, QTcpSocket *socket);
    void recvClientCmdFriendList(QString recvData, QTcpSocket *socket);
    void recvClientCmdTalkSend(QString recvData);
    void recvClientCmdTalkRecv(QString recvData, QTcpSocket *socket);
    void recvClientCmdPicSend();
    void recvClientCmdPicMeta(QTcpSocket *socket);
    void recvClientCmdPicRecv(QString recvData, QTcpSocket *socket);
};

#endif // SERVER_H
