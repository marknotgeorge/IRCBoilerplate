#ifndef CONNECTION_H
#define CONNECTION_H

#include <QString>

class Connection
{
public:
    Connection();

    QString host;
    int port;
    QString nickName;
    QString username;
    QString password;
    QString realName;
};

#endif // CONNECTION_H
