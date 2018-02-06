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
    /*
    QDateTime timestamp;
    QString mytime;

    mytime = timestamp.currentDateTime().toString();
    qDebug() << mytime;
    */

    QVBoxLayout *mainLayout = nullptr;
    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(statusLabel);

    sessionOpened();

    connect(tcpServer, &QTcpServer::newConnection, this, &Server::sendTimeStamp);
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
                            "Run the Fortune Client example now.")
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

void Server::readData()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    in.setDevice(socket);
    in.setVersion(QDataStream::Qt_5_9);

    /*
    in.startTransaction();
    QString nextFortune;
    in >> nextFortune;

    if (!in.commitTransaction()) {
        return;
    }

    qDebug() << "Server receive" << nextFortune;
    */

    if (cmdID == 0) {
        if (socket->bytesAvailable() < (int)sizeof(quint64))
            return;
        in >> cmdID;
        qDebug() << "cmdid" <<cmdID;
    }

    if (blockSize == 0) {
        if (socket->bytesAvailable() < (int)sizeof(quint64))
            return;
        in >> blockSize;
        qDebug() << blockSize;
    }

    if (cmdID == 6) {
        if (clientUid == 0) {
            if (socket->bytesAvailable() < (int)sizeof(quint64))
                return;
            in >> clientUid;
            qDebug() << clientUid;
        }

        if (socket->bytesAvailable() < blockSize)
            return;

        QByteArray nextByte;
        in >> nextByte;
        QString clientName;
        for (int i = 0; i < clientVector.count(); i++)
            if (clientVector[i].uid == clientUid) {
                clientName = clientVector[i].name;
                break;
            }

        QString clientFolder = picFolder + clientName;
        QDir dir(clientFolder);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QString clientFile = clientFolder + "/" + clientName + ".jpg";
        qDebug() << "client file " << clientFile;
        QFile file(clientFile);
        file.open(QIODevice::WriteOnly);
        file.write(nextByte);
        file.close();

        clientUid = 0;
    }
    else {
        if (socket->bytesAvailable() < blockSize)
            return;

        QString myData;
        in >> myData;
        //qDebug() << "myData " << myData;

        if (cmdID == 1 || cmdID == 2) {
            QString name = myData.split(" ").at(0);
            QString password = myData.split(" ").at(1);
            //qDebug() << name;
            //qDebug() << password;

            if (cmdID == 1) {
                for (int i = 0; i < clientVector.count(); ++i)
                    if (!clientVector[i].name.compare(name))
                        sendReturn(socket, cmdID, "0|User name " + name +" already exist|" );

                struct clientInfo tempInfo;
                tempInfo.name = name;
                tempInfo.passwd = password;
                tempInfo.uid = uid;
                tempInfo.messageBox.clear();
                uid++;

                clientVector.push_back(tempInfo);

                /*
                for (int i = 0; i < clientVector.count(); ++i)
                    qDebug() << clientVector[i].name << " " << clientVector[i].passwd << " "<<clientVector[i].uid;
                */

                sendReturn(socket, cmdID, QString::number(uid-1) + "|Success add user : " + name + "|");
            }

            if (cmdID == 2) {
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
        }
        if (cmdID == 3) {
            int uid = myData.toInt();
            QString returnValue = "";
            returnValue = returnValue + QString::number(clientVector.count() - 1) + " ";
            for (int i = 0; i < clientVector.count(); ++i) {
                if (clientVector[i].uid != uid) {
                    returnValue = returnValue + clientVector[i].name + " " + QString::number(clientVector[i].uid) + " ";
                }
            }
            sendReturn(socket, cmdID, returnValue);
        }

        if (cmdID == 4) {
            QString sendUidString = myData.split(" ").at(0);
            QString recvUidString = myData.split(" ").at(1);
            int sendUid = sendUidString.toInt();
            int recvUid = recvUidString.toInt();
            QString messageText = myData.split("\n").at(1);

            qDebug() << "ID 4 " << sendUidString << recvUidString << messageText;
            for (int i = 0; i < clientVector.count(); i++) {
                if (clientVector[i].uid == recvUid) {
                    struct message tempMessage;
                    tempMessage.text = messageText;
                    tempMessage.uid = sendUid;
                    clientVector[i].messageBox.push_back(tempMessage);
                    break;
                }
            }
        }

        if (cmdID == 5) {
            int uid = myData.toInt();
            int msgNum = 0;
            int senderUid;
            QString senderMsg;
            QString clientMsg = "";
            for (int i = 0; i < clientVector.count(); i++) {
                if (uid == clientVector[i].uid) {
                    msgNum = clientVector[i].messageBox.count();
                    if (msgNum > 0) {
                        clientMsg = clientMsg + QString::number(msgNum) + "\n";
                        for (int j = 0; j < msgNum; j++) {
                            senderUid = clientVector[i].messageBox[j].uid;
                            senderMsg = clientVector[i].messageBox[j].text;
                            clientMsg = clientMsg + QString::number(senderUid) + "\n" + senderMsg + "\n";
                        }
                        qDebug() << "send 5 " << clientMsg;
                        sendReturn(socket, cmdID, clientMsg);
                        clientVector[i].messageBox.clear();
                    }
                    break;
                }
            }
        }
    }

    cmdID = 0;
    blockSize = 0;
    /*
    QByteArray nextByte;
    in >> nextByte;

    QFile file("C:/Users/A60013/Pictures/temp/new.jpg");
    file.open(QIODevice::WriteOnly);
    file.write(nextByte);
    file.close();
    */
}

void Server::sendTimeStamp()
{
    /*
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    QDateTime timestamp;
    QString mytime;

    mytime = timestamp.currentDateTime().toString();
    out << mytime;
    */
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, &QAbstractSocket::disconnected,
            clientConnection, &QObject::deleteLater);
    connect(clientConnection, &QIODevice::readyRead, this, &Server::readData);

    //clientConnection->write(block);


    //clientConnection->disconnectFromHost();
}

Server::~Server()
{
    delete ui;
}
