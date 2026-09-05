#include "multiviewplayer.h"
#include <QApplication>
#include <QPalette>
#include <QFile>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QDir>

// 自定义 Application 类来处理文件打开事件
class VideoPlayerApplication : public QApplication
{
public:
    VideoPlayerApplication(int &argc, char **argv) : QApplication(argc, argv)
    {
        m_player = nullptr;
    }

    void setPlayer(MultiViewPlayer *player)
    {
        m_player = player;
    }

protected:
    bool event(QEvent *event) override
    {
        // macOS 和某些 Linux 桌面环境通过 FileOpen 事件打开文件
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            if (m_player && !openEvent->file().isEmpty()) {
                qDebug() << "FileOpen event received:" << openEvent->file();
                m_player->addVideoWidget(openEvent->file());
                // 激活窗口
                m_player->showNormal();
                m_player->raise();
                m_player->activateWindow();
                return true;
            }
        }
        return QApplication::event(event);
    }

private:
    MultiViewPlayer *m_player;
};

int main(int argc, char *argv[])
{
    // 设置应用程序属性（在创建 QApplication 之前）
    QApplication::setApplicationName("MultiView Video Player");
    QApplication::setOrganizationName("VideoPlayer");

    VideoPlayerApplication app(argc, argv);

    // 设置调色板
    QPalette palette = app.palette();
    palette.setColor(QPalette::Active, QPalette::Highlight, QColor(0, 120, 215));
    palette.setColor(QPalette::Active, QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0, 120, 215));
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);

    // 单实例检查 - 使用更可靠的服务器名称
    const QString serverName = QString("MultiViewVideoPlayer_%1").arg(QDir::homePath().toUtf8().toBase64());
    QLocalSocket socket;
    socket.connectToServer(serverName);

    // 增加等待时间，提高可靠性
    if (socket.waitForConnected(1000)) {
        // 已有实例在运行，发送文件路径到现有实例
        QStringList args = app.arguments();
        QStringList filesToSend;

        // 收集所有要发送的文件
        for (int i = 1; i < args.size(); ++i) {
            QString filePath = args.at(i);
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                filesToSend.append(filePath);
            }
        }

        if (!filesToSend.isEmpty()) {
            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            out << filesToSend;

            socket.write(data);
            if (!socket.waitForBytesWritten(3000)) {
                qWarning() << "Failed to send files to existing instance";
            }
        }

        socket.disconnectFromServer();
        qDebug() << "Another instance is running, exiting...";
        return 0;  // 退出新实例
    }

    // 检查并处理本地服务器错误
    QLocalServer *localServer = new QLocalServer();

    // 如果地址已被占用，尝试清理
    QLocalServer::removeServer(serverName);

    if (!localServer->listen(serverName)) {
        qWarning() << "Failed to create local server:" << localServer->errorString();
        // 可能已有实例在运行，尝试连接发送文件后退出
        QLocalSocket retrySocket;
        retrySocket.connectToServer(serverName);
        if (retrySocket.waitForConnected(1000)) {
            QStringList args = app.arguments();
            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            QStringList files;
            for (int i = 1; i < args.size(); ++i) {
                if (QFile::exists(args[i])) files.append(args[i]);
            }
            out << files;
            retrySocket.write(data);
            retrySocket.waitForBytesWritten(3000);
            retrySocket.disconnectFromServer();
        }
        delete localServer;
        return 0;
    }

    MultiViewPlayer player;
    player.setWindowTitle("MultiView Video Player");
    player.resize(1280, 610);
    player.setMinimumSize(640, 480);

    // 设置玩家引用到 Application
    app.setPlayer(&player);
    player.show();

    // 处理来自其他实例的文件
    QObject::connect(localServer, &QLocalServer::newConnection, [&player, localServer]() {
        QLocalSocket *clientSocket = localServer->nextPendingConnection();
        if (clientSocket) {
            // 等待数据准备好
            if (clientSocket->waitForReadyRead(3000)) {
                QByteArray data = clientSocket->readAll();
                QDataStream in(&data, QIODevice::ReadOnly);
                QStringList files;
                in >> files;

                qDebug() << "Received" << files.size() << "files from another instance";

                for (const QString &filePath : files) {
                    if (QFile::exists(filePath)) {
                        qDebug() << "Adding file:" << filePath;
                        player.addVideoWidget(filePath);
                    }
                }

                // 激活窗口（确保窗口可见并在前台）
                player.showNormal();
                player.raise();
                player.activateWindow();
            }

            clientSocket->disconnectFromServer();
            clientSocket->deleteLater();
        }
    });

    // 处理命令行参数（首次启动）
    QStringList args = app.arguments();
    qDebug() << "Command line arguments:" << args;

    bool hasFiles = false;
    for (int i = 1; i < args.size(); ++i) {
        QString filePath = args.at(i);
        if (QFile::exists(filePath)) {
            qDebug() << "Opening file from command line:" << filePath;
            player.addVideoWidget(filePath);
            hasFiles = true;
        }
    }

    // 如果没有打开任何文件，可以显示一个提示或空状态
    if (!hasFiles) {
        qDebug() << "No files specified, showing empty player";
    }

    int result = app.exec();

    // 清理服务器
    localServer->close();
    delete localServer;

    return result;
}
