#include "server.h"
#include "ui_server.h"
#include <QtWidgets>
#include <QtCore>

Server::Server(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Server),
    statusLabel(new QLabel)
{
    ui->setupUi(this);

    QVBoxLayout *mainLayout = nullptr;
    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(statusLabel);

    sessionOpened();

    connect(tcpServer, &QTcpServer::newConnection, this, &Server::handleNewConn);
}

void Server::sessionOpened()
{
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 54321)) {
        QMessageBox::critical(this, tr("Fortune Server"),
                              tr("Unable to start the server: %1.")
                              .arg(tcpServer->errorString()));
        close();
        return;
    }
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    statusLabel->setText(tr("The server is running on\n\nIP: %1\nport: %2\n\n"
                            "Run the Messanger now.")
                         .arg(ipAddress).arg(tcpServer->serverPort()));
}

void Server::sendReturn(QTcpSocket *socket, quint64 id, QString message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    //qDebug() << message;

    out << (quint64)0;
    out << (quint64)0;
    out << message;
    out.device()->seek(0);
    out << id;
    out.device()->seek(sizeof(quint64));
    out << (quint64)(block.size() - (2 * sizeof(quint64)));

    socket->write(block);
    socket->waitForBytesWritten();
}

void Server::recvClientCmdRegister(QString recvData, QTcpSocket *socket)
{
    QString name = recvData.split(" ").at(0);
    QString password = recvData.split(" ").at(1);
    for (int i = 0; i < clientVector.count(); ++i)
        if (!clientVector[i].name.compare(name))
            sendReturn(socket, cmdID, "0|User name " + name +" already exist|" );

    struct clientInfo tempInfo;
    tempInfo.name = name;
    tempInfo.passwd = password;
    tempInfo.uid = uid;
    tempInfo.messageBox.clear();
    tempInfo.hasClientIcon = false;
    uid++;

    clientVector.push_back(tempInfo);

    sendReturn(socket, cmdID, QString::number(uid-1) + "|Success add user : " + name + "|");
}

void Server::recvClientCmdLogin(QString recvData, QTcpSocket *socket)
{
    QString name = recvData.split(" ").at(0);
    QString password = recvData.split(" ").at(1);
    bool checking = false;
    int uid;
    for (int i = 0; i < clientVector.count(); ++i) {
        if ((!clientVector[i].name.compare(name)) && (!clientVector[i].passwd.compare(password))) {
            checking = true;
            uid = clientVector[i].uid;
            break;
        }
    }
    if (checking)
        sendReturn(socket, cmdID, "yes " + QString::number(uid) + " ");
    else
        sendReturn(socket, cmdID, "no 0");
}

void Server::recvClientCmdFriendList(QString recvData, QTcpSocket *socket)
{
    int uid = recvData.toInt();
    QString returnValue = "";
    returnValue = returnValue + QString::number(clientVector.count() - 1) + " ";
    for (int i = 0; i < clientVector.count(); ++i) {
        if (clientVector[i].uid != uid) {
            returnValue = returnValue + clientVector[i].name + " " + QString::number(clientVector[i].uid) + " ";
        }
    }
    sendReturn(socket, cmdID, returnValue);
}

void Server::recvClientCmdTalkSend(QString recvData)
{
    QString sendUidString = recvData.split(" ").at(0);
    QString recvUidString = recvData.split(" ").at(1);
    int sendUid = sendUidString.toInt();
    int recvUid = recvUidString.toInt();
    QString messageText = recvData.split("\n").at(1);

    qDebug() << "ID 4 " << sendUidString << recvUidString << messageText;
    for (int i = 0; i < clientVector.count(); i++) {
        if (clientVector[i].uid == recvUid) {
            struct message tempMessage;
            tempMessage.text = messageText;
            tempMessage.uid = sendUid;
            tempMessage.isText = true;
            clientVector[i].messageBox.push_back(tempMessage);
            break;
        }
    }
}

void Server::recvClientCmdTalkRecv(QString recvData, QTcpSocket *socket)
{
    int uid = recvData.toInt();
    int msgNum = 0, textNum = 0, gifNum = 0;
    int senderUid;
    QString senderMsg, senderGif;
    QString clientMsgText = "";
    QString clientMsgGif = "";

    for (int i = 0; i < clientVector.count(); i++) {
        if (uid == clientVector[i].uid) {
            msgNum = clientVector[i].messageBox.count();
            if (msgNum > 0) {
                for (int j = 0; j < msgNum; j++) {
                    senderUid = clientVector[i].messageBox[j].uid;

                    if (clientVector[i].messageBox[j].isText == true) {
                        senderMsg = clientVector[i].messageBox[j].text;
                        clientMsgText = clientMsgText + QString::number(senderUid) + "\n" + senderMsg + "\n";
                        textNum++;
                    } else {
                        qDebug() << "send gif ";
                        senderGif = QString::number(clientVector[i].messageBox[j].gifNum);
                        clientMsgGif = clientMsgGif + QString::number(senderUid) + "\n" + senderGif + "\n";
                        gifNum++;
                    }
                }

                if (textNum > 0) {
                    clientMsgText = QString::number(textNum) + "\n" + clientMsgText;
                    qDebug() << "send 5 " << clientMsgText;
                    sendReturn(socket, cmdID, clientMsgText);
                }

                if (gifNum > 0) {
                    clientMsgGif = QString::number(gifNum) + "\n" + clientMsgGif;
                    qDebug() << "send 6 " << clientMsgGif;
                    sendReturn(socket, GifRecv, clientMsgGif);
                }
            }
            clientVector[i].messageBox.clear();
            break;
        }
    }

}

void Server::recvClientCmdPicSend()
{
    QByteArray nextByte;
    in >> nextByte;
    for (int i = 0; i < clientVector.count(); i++)
        if (clientVector[i].uid == (int)clientUid) {
            clientVector[i].hasClientIcon = true;
            break;
        }

    QDir dir(picFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString clientFile = picFolder + QString::number((int)clientUid) + ".jpg";
    qDebug() << "client file " << clientFile;
    QFile file(clientFile);
    file.open(QIODevice::WriteOnly);
    file.write(nextByte);
    file.close();

    clientUid = 0;
}

void Server::recvClientCmdPicMeta(QTcpSocket *socket)
{
    QString clientMsg = "";
    int picNumber = 0;
    for (int i = 0; i < clientVector.count(); ++i) {
        if (clientVector[i].hasClientIcon) {
            clientMsg = clientMsg + " " + QString::number(clientVector[i].uid);
            picNumber++;
        }
    }

    if (picNumber > 0) {
        clientMsg = QString::number(picNumber) + clientMsg + " ";
        sendReturn(socket, cmdID, clientMsg);
    }
}

void Server::recvClientCmdPicRecv(QString recvData, QTcpSocket *socket)
{
    int uid = recvData.toInt();
    qDebug() << "cmdid 8 =========== send pic" << uid;
    QString clientFile = picFolder + "/" + QString::number(uid) + ".jpg";
    sendNetworkfile(clientFile, socket, uid);
}

void Server::recvClientCmdGifSend(QString recvData)
{
    QString sendUidString = recvData.split(" ").at(0);
    QString recvUidString = recvData.split(" ").at(1);
    int sendUid = sendUidString.toInt();
    int recvUid = recvUidString.toInt();
    QString gifNum = recvData.split("\n").at(1);

    qDebug() << "gif recv " << sendUidString << recvUidString << gifNum;
    for (int i = 0; i < clientVector.count(); i++) {
        if (clientVector[i].uid == recvUid) {
            struct message tempMessage;
            tempMessage.gifNum = gifNum.toInt();
            tempMessage.uid = sendUid;
            tempMessage.isText = false;
            clientVector[i].messageBox.push_back(tempMessage);
            break;
        }
    }
}

void Server::readData()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    in.setDevice(socket);
    in.setVersion(QDataStream::Qt_5_9);
    bool done = false;

    while (!done) {
        if (cmdID == None) {
            if (socket->bytesAvailable() < (qint64)sizeof(quint64))
                return;
            in >> cmdID;
            qDebug() << "cmdid" <<cmdID;
        }

        if (blockSize == 0) {
            if (socket->bytesAvailable() < (qint64)sizeof(quint64))
                return;
            in >> blockSize;
            qDebug() << blockSize;
        }

        if (cmdID == PicSend) {
            if (clientUid == 0) {
                if (socket->bytesAvailable() < (qint64)sizeof(quint64))
                    return;
                in >> clientUid;
                qDebug() << clientUid;
            }

            if (socket->bytesAvailable() < (qint64)blockSize)
                return;

            recvClientCmdPicSend();
        }
        else {
            if (socket->bytesAvailable() < (qint64)blockSize)
                return;

            QString recvData;
            in >> recvData;

            if (cmdID == Register) {
                recvClientCmdRegister(recvData, socket);
            }

            if (cmdID == Login) {
                recvClientCmdLogin(recvData, socket);
            }

            if (cmdID == FriendList) {
                recvClientCmdFriendList(recvData, socket);
            }

            if (cmdID == TalkSend) {
                recvClientCmdTalkSend(recvData);
            }

            if (cmdID == TalkRecv) {
                recvClientCmdTalkRecv(recvData, socket);
            }

            if (cmdID == PicMeta) {
                recvClientCmdPicMeta(socket);
            }

            if (cmdID == PicRecv) {
                recvClientCmdPicRecv(recvData, socket);
            }

            if (cmdID == GifSend) {
                recvClientCmdGifSend(recvData);
            }

        }

        cmdID = None;
        blockSize = 0;

        if (socket->bytesAvailable() == 0)
            done = true;
        else
            qDebug() << "More data to read";
    }
}

void Server::sendNetworkfile(QString filePath, QTcpSocket *socket, int uid)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        qDebug() << "open file path " + filePath + "failed";
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    out << (quint64)0;
    out << (quint64)0;
    out << (quint64)0;
    out << file.readAll();
    out.device()->seek(0);
    out << (quint64)8;
    out.device()->seek(sizeof(quint64));
    out << (quint64)(block.size() - (3 * sizeof(quint64)));
    out.device()->seek(2 * sizeof(quint64));
    out << (quint64)uid;

    socket->write(block);
    socket->waitForBytesWritten();
}

void Server::handleNewConn()
{
    qDebug() << "handleNewConn";
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();

    connect(clientConnection, &QAbstractSocket::disconnected,
            clientConnection, &QObject::deleteLater);

    connect(clientConnection, &QIODevice::readyRead, this, &Server::readData);
}

Server::~Server()
{
    delete ui;
}
