#ifndef SESSION_H
#define SESSION_H

#include <IrcSession>
#include <QStringList>
#include <IrcMessage>
#include "connection.h"

class Session : public IrcSession
{
    Q_OBJECT

public:
    Session(QObject *parent = 0);
    Connection *connectionSettings;


signals:
    void outputString(QString output);
    void newNamesList(QString channel, QStringList nameList);
    void newChannelList(QStringList channelList);

public slots:
    void onConnected();
    void onMessageReceived(IrcMessage *message);
    void onInputReceived(QString channel, QString input);
    void onRefreshNames(QString channel);
    void onUpdateConnection(Connection *connection);
    void onPassword(QString *password);

private:
    QStringList nicknameList;

    // Command parsing instructions...
private:
    static IrcCommand* parseAway(const QString& channel, const QStringList& params);
    static IrcCommand* parseInvite(const QString& channel, const QStringList& params);
    static IrcCommand* parseJoin(const QString& channel, const QStringList& params);
    static IrcCommand* parseKick(const QString& channel, const QStringList& params);
    static IrcCommand* parseMe(const QString& channel, const QStringList& params);
    static IrcCommand* parseMode(const QString& channel, const QStringList& params);
    static IrcCommand* parseNames(const QString& channel, const QStringList& params);
    static IrcCommand* parseNick(const QString& channel, const QStringList& params);
    static IrcCommand* parseNotice(const QString& channel, const QStringList& params);
    static IrcCommand* parsePart(const QString& channel, const QStringList& params);
    static IrcCommand* parsePing(const QString& channel, const QStringList& params);
    static IrcCommand* parseQuit(const QString& channel, const QStringList& params);
    static IrcCommand* parseQuote(const QString& channel, const QStringList& params);
    static IrcCommand* parseTime(const QString& channel, const QStringList& params);
    static IrcCommand* parseTopic(const QString& channel, const QStringList& params);
    static IrcCommand* parseVersion(const QString& channel, const QStringList& params);
    static IrcCommand* parseWhois(const QString& channel, const QStringList& params);
    static IrcCommand* parseWhowas(const QString& channel, const QStringList& params);

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

    QString prettyUser(const IrcSender& sender);
    QString prettyUser(const QString& user);
    QString colorize(const QString& str, const QString& colourName);
    QString formatPingReply(const IrcSender& sender, const QString& arg);

};

#endif // SESSION_H
