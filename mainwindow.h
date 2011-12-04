#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <IrcMessage>
#include <QTimer>
#include "sessionwrapper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void printToOutputArea(QString output);
    Session *chatSession;

signals:
    void sendInputString(QString channel,QString input);
    void refreshNames(QString channel);
    void updateSettings(Connection *connectionSettings);

private slots:
    void on_joinButton_clicked();

    void on_aboutButton_clicked();
    void onOutputStringReceived(QString output);
    void onNewNamesList(QString channel, QStringList namesList);
    void addToNamesList(const QString& user);
    void removeFromNamesList(const QString& user);

    void on_inputEdit_returnPressed();
    void closeEvent(QCloseEvent *event);

    void on_getNamesButton_clicked();
    void onSessionConnected();
    void onSessionDisconnected();

    void on_connectButton_clicked();

    void onMessageReceived(IrcMessage *message);
    void onConnectionTimeout();



    // Message Handling functions...
protected:
    void handleInviteMessage(IrcInviteMessage* message);
    void handleJoinMessage(IrcJoinMessage* message);
    void handleKickMessage(IrcKickMessage* message);
    void handleModeMessage(IrcModeMessage* message);
    void handleNickMessage(IrcNickMessage* message);
    void handleNoticeMessage(IrcNoticeMessage* message);
    void handleNumericMessage(IrcNumericMessage* message);
    void handlePartMessage(IrcPartMessage* message);
    void handlePongMessage(IrcPongMessage* message);
    void handlePrivateMessage(IrcPrivateMessage* message);
    void handleQuitMessage(IrcQuitMessage* message);
    void handleTopicMessage(IrcTopicMessage* message);
    void handleUnknownMessage(IrcMessage* message);

private:
    Ui::MainWindow *ui;
    QStringList nicknameList;
    QString baseWindowTitle;
    QString chatChannel;
    QTimer *connectionTimeout;





};

#endif // MAINWINDOW_H
