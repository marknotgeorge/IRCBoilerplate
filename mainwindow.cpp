#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sessionwrapper.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <Irc>
#include <IrcUtil>
#include <QIntValidator>
#include <QStringList>
#include <IrcMessage>
#include <QCloseEvent>
#include <QDateTime>
#include <IrcCommand>
#include <QList>
#include <QTimer>
#include <QDebug>
#include <QHash>
#include <QTextEdit>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    

    // Create the window title from the BUILD define, which is created
    // from the Git describe string...
    QString versionString = BUILD;
    baseWindowTitle = tr("IRCBoilerplate %1 ").arg(versionString);
    this->setWindowTitle(baseWindowTitle);

    // Load the settings into the edit boxes...
    QSettings settings("marknotgeorge", "IRCBoilerplate", this);

    ui->serverEdit->setText(settings.value("server", "irc.server.com").toString());
    ui->portEdit->setText(settings.value("port", "6667").toString());
    ui->nicknameEdit->setText(settings.value("nickname", "wibble").toString());
    ui->passwordEdit->setText(settings.value("password","password").toString());
    ui->usernameEdit->setText(settings.value("username", "hatstand").toString());
    ui->realnameEdit->setText(settings.value("realname", "Dante Hicks").toString());



    // Create the Session...
    chatSession = new Session(this);
    connectionTimeout = new QTimer(this);

    // Send it to the session object...
    Connection *connectionSettings = new Connection;

    connectionSettings->host = ui->serverEdit->text();
    connectionSettings->port = ui->portEdit->text().toInt();
    connectionSettings->nickName = ui->nicknameEdit->text();
    connectionSettings->username = ui->usernameEdit->text();
    connectionSettings->realName= ui->realnameEdit->text();
    connectionSettings->password = ui->passwordEdit->text();
    emit updateSettings(connectionSettings);
    delete connectionSettings;

    // Create a timeout

    // Connect signals to slots...
    connect(chatSession, SIGNAL(outputString(QString, QString )), this, SLOT(onOutputStringReceived(QString, QString)));
    connect(chatSession, SIGNAL(newNamesList(QString,QStringList)), this, SLOT(onNewNamesList(QString,QStringList)));
    connect(chatSession, SIGNAL(connected()), this, SLOT(onSessionConnected()));
    connect(chatSession, SIGNAL(disconnected()),this, SLOT(onSessionDisconnected()));
    connect(chatSession, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
    connect(chatSession, SIGNAL(socketError(QAbstractSocket::SocketError)),this, SLOT(onConnectionTimeout()));
    connect(connectionTimeout, SIGNAL(timeout()), this, SLOT(onConnectionTimeout()));

    // Create a validator so that portEdit can only accept an integer
    // between 0 and 65535...
    QIntValidator *validator = new QIntValidator(0, 65535, this);

    ui->portEdit->setValidator(validator);

    // initialise chatChannel
    chatChannel.clear();

    // Delete all the tabs created by the designer in outputTabWidget.
    // Clunky, but I know no other way...
    ui->outputTabWidget->clear();

    // Set up the server output tab...
    QTextEdit *editor = new QTextEdit(ui->outputTabWidget);
    editor->setReadOnly(false);
    ui->outputTabWidget->addTab(editor, "server");
    channelHash.insert("server", editor);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_joinButton_clicked()
{

        bool ok;
        QString channel = QInputDialog::getText(this, tr("Join Channel"), tr("Channel name:"), QLineEdit::Normal, "", &ok);

        if (ok && !channel.isEmpty())
        {
            IrcCommand *command;
            command = IrcCommand::createJoin(channel);
            ui->statusBar->showMessage(tr("Joining %1...").arg(channel));
            chatSession->sendCommand(command);
        }

}

void MainWindow::on_aboutButton_clicked()
{
    // Display an About box. This uses the BUILD macro created from the
    // Git describe string in IRCBoilerplate.pro

    QString versionString = BUILD;
    QString aboutString = "IRC BoilerPlate " + versionString + "\n ©2011 Mark Johnson.\nUses the Communi IRC library by J-P Nurmi et al.";

    QMessageBox::about(this, tr("About IRCBoilerplate"), aboutString);
}

void MainWindow::printToOutputArea(QString channel, QString output)
{
    if(channel == chatSession->host())
        channelHash["server"]->append(output);
    else if (channelHash[channel])
        channelHash[channel]->append(output);
}

void MainWindow::on_inputEdit_returnPressed()
{
    // Return pressed, so emit a sendInputString() signal...
    QString input = ui->inputEdit->text();

    if (!input.isEmpty())
    {
        emit sendInputString(chatChannel, input);
        ui->inputEdit->clear();
    }
}

void MainWindow::onOutputStringReceived(QString channel, QString output)
{
    printToOutputArea(channel, output);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Check that we really want to quit if still connected.
    if (chatSession->isActive())
    {
        int response = QMessageBox::warning(this, "IRCBoilerPlate",
                                            tr("You are still connected!\n Do you really want to quit?"),
                                            QMessageBox::Yes, QMessageBox::No);
        if (response == QMessageBox::No)
            event->ignore();

    }
    QSettings settings("marknotgeorge", "IRCBoilerplate");



    // Remove the validator as I think it causes problems saving
    // settings...
    ui->portEdit->setValidator(0);

    // Save the connection settings...
    settings.setValue("server", ui->serverEdit->text());
    settings.setValue("port", ui->portEdit->text());
    settings.setValue("nickname", ui->nicknameEdit->text());
    settings.setValue("username", ui->usernameEdit->text());
    settings.setValue("password", ui->passwordEdit->text());
    settings.setValue("realname", ui->realnameEdit->text());

}

void MainWindow::onNewNamesList(QString channel, QStringList namesList)
{
    // A new names list has appeared! insert it into the hash table.
    namesListHash.insert(channel, namesList);

    // If the channel is the current one, replace the list in the
    // nicknames ListWidget
    if (channel == chatChannel)
    {
        ui->namesListWidget->clear();
        ui->namesListWidget->addItems(namesList);
    }

}



void MainWindow::on_getNamesButton_clicked()
{
    // Refresh the names list for the current channel.

    emit refreshNames(chatChannel);
}

void MainWindow::onSessionConnected()
{
    // Stop the cconnection timeout timer...
    connectionTimeout->stop();

    //Show a message in the StatusBar...
    QString message = tr("Connected to %1");
    ui->statusBar->showMessage(message.arg(ui->serverEdit->text()));

    //Rename the connect button to Disconnect...
    ui->connectButton->setEnabled(true);
    ui->connectButton->setText(tr("Disconnect"));

    // Alter the window title to show the connection...
    this->setWindowTitle(baseWindowTitle + tr("Connected to %1 ").arg(chatSession->host()));

    // Enable the join channel button
    ui->joinButton->setEnabled(true);

}

void MainWindow::on_connectButton_clicked()
{
    // If the sessions is not active, we must have clicked Connect.
    if (!chatSession->isActive())
    {
        Connection *connectionSettings = new Connection;

        // Transfer the connection settings...
        connectionSettings->host = ui->serverEdit->text();
        connectionSettings->port = ui->portEdit->text().toInt();
        connectionSettings->nickName = ui->nicknameEdit->text();
        connectionSettings->username = ui->usernameEdit->text();
        connectionSettings->realName= ui->realnameEdit->text();
        connectionSettings->password = ui->passwordEdit->text();
        emit updateSettings(connectionSettings);
        delete connectionSettings;

        chatSession->open();

        // start the timeout timer
        connectionTimeout->setSingleShot(true);
        connectionTimeout->setInterval(15000);
        connectionTimeout->start();

        // Disable the connect button and change the name to Connecting...
        ui->connectButton->setEnabled(false);
        ui->connectButton->setText(tr("Connecting..."));


    }
    else // We must have clicked Disconnect.
    {
        int response = QMessageBox::question(this, "IRCBoilerplate",
                                             tr("Are you sure you wish to disconnect from the server?"),
                                             QMessageBox::Yes, QMessageBox::No);
        if (response == QMessageBox::Yes)
            chatSession->close();

    }
}

void MainWindow::onSessionDisconnected()
{
    //Show a message in the StatusBar...
    QString message = tr("Disconnected from %1");
    ui->statusBar->showMessage(message.arg(ui->serverEdit->text()));

    //Rename the connect button to Disconnect...
    ui->connectButton->setText(tr("Connect"));

    // Alter the window title
    this->setWindowTitle(baseWindowTitle);

    // Clear the message window
    // ui->outputArea->clear();

    // Clear and disable the names list area and the join channel button...
    ui->namesListWidget->clear();
    ui->namesListWidget->setEnabled(false);
    ui->getNamesButton->setEnabled(false);
    ui->joinButton->setText(tr("Join Channel..."));
    ui->joinButton->setEnabled(false);


}

void MainWindow::onMessageReceived(IrcMessage *message)
{
    switch (message->type())
        {
        case IrcMessage::Invite:
            handleInviteMessage(static_cast<IrcInviteMessage*>(message));
            break;
        case IrcMessage::Join:
            handleJoinMessage(static_cast<IrcJoinMessage*>(message));
            break;
        case IrcMessage::Kick:
            handleKickMessage(static_cast<IrcKickMessage*>(message));
            break;
        case IrcMessage::Mode:
            handleModeMessage(static_cast<IrcModeMessage*>(message));
            break;
        case IrcMessage::Nick:
            handleNickMessage(static_cast<IrcNickMessage*>(message));
            break;
        case IrcMessage::Notice:
            handleNoticeMessage(static_cast<IrcNoticeMessage*>(message));
            break;
        case IrcMessage::Numeric:
            handleNumericMessage(static_cast<IrcNumericMessage*>(message));
            break;
        case IrcMessage::Part:
            handlePartMessage(static_cast<IrcPartMessage*>(message));
            break;
        case IrcMessage::Pong:
            handlePongMessage(static_cast<IrcPongMessage*>(message));
            break;
        case IrcMessage::Private:
            handlePrivateMessage(static_cast<IrcPrivateMessage*>(message));
            break;
        case IrcMessage::Quit:
            handleQuitMessage(static_cast<IrcQuitMessage*>(message));
            break;
        case IrcMessage::Topic:
            handleTopicMessage(static_cast<IrcTopicMessage*>(message));
            break;
        case IrcMessage::Unknown:
            handleUnknownMessage(static_cast<IrcMessage*>(message));
            break;
        default:
            break;
    }
}

void MainWindow::handleInviteMessage(IrcInviteMessage *message)
{
    // printToOutputArea("Invite Message Received!");
}

void MainWindow::handleJoinMessage(IrcJoinMessage *message)
{
    IrcSender sender = message->sender();
    // Check to see if we are the one that's joining...
    if (sender.name() == ui->nicknameEdit->text())
    {
        // Set statusbar...
        ui->statusBar->showMessage(tr("Joined channel %1").arg(message->channel()));



        // Set Join channel button to part from channel...
        //ui->joinButton->setText(tr("Leave %1").arg(message->channel()));
        //chatChannel = message->channel();
        //ui->namesListWidget->setEnabled(true);
        //ui->getNamesButton->setEnabled(true);

        // Add channel output tab.
        if(!channelHash[message->channel()])
        {
            QTextEdit *editor = new QTextEdit(ui->outputTabWidget);
            editor->setReadOnly(false);
            ui->outputTabWidget->addTab(editor, message->channel());
            channelHash.insert(message->channel(), editor);
            ui->outputTabWidget->setCurrentWidget(editor);
            chatChannel = message->channel();
        }
    }
    else
        addToNamesList(message->channel(),sender.name());

}

void MainWindow::handleKickMessage(IrcKickMessage *message)
{
    // printToOutputArea("Kick message received!");
}

void MainWindow::handleModeMessage(IrcModeMessage *message)
{
    // printToOutputArea("Mode message received");
}

void MainWindow::handleNickMessage(IrcNickMessage *message)
{
    // printToOutputArea("Nick message received!");

}

void MainWindow::handleNoticeMessage(IrcNoticeMessage *message)
{
    // printToOutputArea("Notice Message Received!");
}


// Definitions used in handleNumericMessage...
#define P_(x) message->parameters().value(x)
#define MID_(x) QStringList(message->parameters().mid(x)).join(" ")

void MainWindow::handleNumericMessage(IrcNumericMessage *message)
{
   /*
    if (message->code() < 300)
        printToOutputArea(tr("[INFO] %1").arg(IrcUtil::messageToHtml(MID_(1))));
    if (QByteArray(Irc::toString(message->code())).startsWith("ERR_"))
        printToOutputArea(tr("[ERROR] %1").arg(IrcUtil::messageToHtml(MID_(1))));

    switch (message->code())
    {
    case Irc::RPL_MOTDSTART:
    case Irc::RPL_MOTD:
        printToOutputArea(tr("[MOTD] %1").arg(IrcUtil::messageToHtml(MID_(1))));
    case Irc::RPL_AWAY:
        printToOutputArea(tr("! %1 is away (%2)").arg(P_(1), MID_(2)));
    case Irc::RPL_ENDOFWHOIS:
        printToOutputArea(QString());
    case Irc::RPL_WHOISOPERATOR:
    case Irc::RPL_WHOISHELPOP: // "is available for help"
    case Irc::RPL_WHOISSPECIAL: // "is identified to services"
    case Irc::RPL_WHOISSECURE: // nick is using a secure connection
    case Irc::RPL_WHOISHOST: // nick is connecting from <...>
        printToOutputArea(tr("! %1").arg(MID_(1)));
    case Irc::RPL_WHOISUSER:
        printToOutputArea(tr("! %1 is %2@%3 (%4)").arg(P_(1), P_(2), P_(3), IrcUtil::messageToHtml(MID_(5))));
    case Irc::RPL_WHOISSERVER:
        printToOutputArea(tr("! %1 is online via %2 (%3)").arg(P_(1), P_(2), P_(3)));
    case Irc::RPL_WHOISACCOUNT: // nick user is logged in as
        printToOutputArea(tr("! %1 %3 %2").arg(P_(1), P_(2), P_(3)));
    case Irc::RPL_WHOWASUSER:
        printToOutputArea(tr("! %1 was %2@%3 %4 %5").arg(P_(1), P_(2), P_(3), P_(4), P_(5)));
    case Irc::RPL_WHOISIDLE: {
        QDateTime signon = QDateTime::fromTime_t(P_(3).toInt());
        QTime idle = QTime().addSecs(P_(2).toInt());
        printToOutputArea(tr("! %1 has been online since %2 (idle for %3)").arg(P_(1),
                                                                                signon.toString(), idle.toString()));
    }
    case Irc::RPL_WHOISCHANNELS:
        printToOutputArea(tr("! %1 is on channels %2").arg(P_(1), P_(2)));
    case Irc::RPL_CHANNELMODEIS:
        printToOutputArea(tr("! %1 mode is %2").arg(P_(1), P_(2)));
    case Irc::RPL_CHANNEL_URL:
        printToOutputArea(tr("! %1 url is %2").arg(P_(1), IrcUtil::messageToHtml(P_(2))));
    case Irc::RPL_CREATIONTIME: {
        QDateTime dateTime = QDateTime::fromTime_t(P_(2).toInt());
        printToOutputArea(tr("! %1 was created %2").arg(P_(1), dateTime.toString()));
    }
    case Irc::RPL_NOTOPIC:
        printToOutputArea(tr("! %1 has no topic set").arg(P_(1)));
    case Irc::RPL_TOPIC:
        printToOutputArea(tr("! %1 topic is \"%2\"").arg(P_(1), IrcUtil::messageToHtml(P_(2))));
    case Irc::RPL_TOPICWHOTIME: {
        QDateTime dateTime = QDateTime::fromTime_t(P_(3).toInt());
        printToOutputArea(tr("! %1 topic was set %2 by %3").arg(P_(1), dateTime.toString(), P_(2)));
    }
    case Irc::RPL_INVITING:
        printToOutputArea(tr("! inviting %1 to %2").arg(P_(1), P_(2)));
    case Irc::RPL_VERSION:
        printToOutputArea(tr("! %1 version is %2").arg(message->sender().name(), P_(1)));
    case Irc::RPL_TIME:
        printToOutputArea(tr("! %1 time is %2").arg(P_(1), P_(2)));
    case Irc::RPL_UNAWAY:
    case Irc::RPL_NOWAWAY:
        printToOutputArea(tr("! %1").arg(P_(1)));

    case Irc::RPL_NAMREPLY:
       foreach (const QString& name, P_(3).split(" ", QString::SkipEmptyParts))
            nicknameList.append(name);
        printToOutputArea(QString());

    case Irc::RPL_ENDOFNAMES: {
        QString channel = P_(1);
        QString msg = tr("! %1 has %2 users").arg(channel).arg(nicknameList.count());
        onNewNamesList(channel, nicknameList);
        printToOutputArea(msg);
    }

    default:
        printToOutputArea(tr("[%1] %2").arg(message->code()).arg(QStringList(message->parameters().mid(1)).join(" ")));
    }
    */
}



void MainWindow::handlePartMessage(IrcPartMessage *message)
{
    IrcSender sender = message->sender();
    removeFromNamesList(message->channel(), sender.name());
}

void MainWindow::handlePongMessage(IrcPongMessage *message)
{
    // printToOutputArea("Pong message received!");
}

void MainWindow::handlePrivateMessage(IrcPrivateMessage *message)
{
    // printToOutputArea("Pong message received!");
}

void MainWindow::handleQuitMessage(IrcQuitMessage *message)
{
    IrcSender sender = message->sender();
    removeFromNamesList(chatChannel, sender.name());
}

void MainWindow::handleTopicMessage(IrcTopicMessage *message)
{
    // printToOutputArea("Topic Message received!");
}

void MainWindow::handleUnknownMessage(IrcMessage *message)
{
    // printToOutputArea("Unknown message received!");
}

void MainWindow::addToNamesList(const QString &channel, const QString &user)
{
    namesListHash[channel].append(user);


    if (channel == chatChannel)
    {
        ui->namesListWidget->addItem(user);
        ui->namesListWidget->sortItems();
    }
}

void MainWindow::removeFromNamesList(const QString &channel, const QString &user)
{
    QString removeUser = user;
    bool removed = namesListHash[channel].removeOne(removeUser);
    if (!removed)
    {
        removeUser.prepend('@');
        removed = namesListHash[channel].removeOne(removeUser);
        if (!removed)
        {
            removeUser.replace('@', '#');
            namesListHash[channel].removeOne(removeUser);
        }
    }

    if (channel == chatChannel)
    {
        if (!user.isEmpty())
        {
            // Try without # or @
            qDebug() << "Removing " << user;
            QList<QListWidgetItem *> results = ui->namesListWidget->findItems(user, Qt::MatchExactly);
            // There should only be one item, so it should be safe to just delete the first one in the list.
            qDebug() << QString("Removing ") << results.count() << QString(" users.");
            if (results.count() != 0)
            {
                delete results.first();
                return;
            }

            // Now try with #
            results.clear();
            QString hashUser = user;
            hashUser.prepend("#");
            qDebug() << "Removing " << hashUser;
            results = ui->namesListWidget->findItems(hashUser, Qt::MatchExactly);
            // There should only be one item, so it should be safe to just delete the first one in the list.
            qDebug() << QString("Removing ") << results.count() << QString(" users.");
            if (results.count() != 0)
            {
                delete results.first();
                return;
            }

            // Now try with #
            results.clear();
            QString atUser = user;
            atUser.prepend("@");
            qDebug() << "Removing " << atUser;
            results = ui->namesListWidget->findItems(atUser, Qt::MatchExactly);
            // There should only be one item, so it should be safe to just delete the first one in the list.
            qDebug() << QString("Removing ") << results.count() << QString(" users.");
            if (results.count() != 0)
            {
                delete results.first();
                return;
            }
        }
    }
}


void MainWindow::onConnectionTimeout()
{
    // The connection timeout has fired, or socketError has fired. This must
    // mean the connection attempt has failed.

    if(connectionTimeout->isActive())
        connectionTimeout->stop();

    ui->statusBar->showMessage(tr("Unable to connect to %1!").arg(chatSession->host()));

    QMessageBox::warning(this, "IRCBoilerplate",
                         tr("The connection to %1 has failed. Please check your settings.")
                         .arg(chatSession->host()), QMessageBox::Ok);
    ui->connectButton->setEnabled(true);
    ui->connectButton->setText(tr("Connect"));

}



void MainWindow::on_outputTabWidget_currentChanged(int index)
{
    // Set chatchannel to the current tab
    chatChannel = ui->outputTabWidget->tabText(index);

    if (chatChannel == "server")
    {
        // This is the server tab, so disable the names list
        ui->namesListWidget->clear();
        ui->namesListWidget->setEnabled(false);
    }
    else
    {
        // This is a normal channel tab, so change the names list
        ui->namesListWidget->setEnabled(true);
        ui->namesListWidget->clear();
        ui->namesListWidget->addItems(namesListHash[chatChannel]);
    }
}

void MainWindow::on_outputTabWidget_tabCloseRequested(int index)
{
    QString channel = ui->outputTabWidget->tabText(index);

    if (channel == "server")
    {
        // We must want to disconnect, but check we're connected first...
        if (chatSession->isActive())
            on_connectButton_clicked();
    }
    else
    {
        // Send the command to leave the channel...
        IrcCommand *command;
        command = IrcCommand::createPart(channel, "Screw you guys, I'm going home!");
        ui->statusBar->showMessage(tr("Leaving %1...").arg(channel));
        chatSession->sendCommand(command);

        // Remove the channel and the entry in the hash table...
        QTextEdit *editor = channelHash[channel];
        ui->outputTabWidget->removeTab(index);
        channelHash.remove(channel);
        delete editor;
    }




}
