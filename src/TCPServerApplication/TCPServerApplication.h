#ifndef TCP_SERVER_APPLICATION_H
#define TCP_SERVER_APPLICATION_H

#include <unordered_map>
#include <memory>
#include <list>

class TCPServerSession;

class TCPServerApplication
{
    friend class TCPServerListenSession;

public:
    TCPServerApplication();
    ~TCPServerApplication();

    TCPServerApplication(const TCPServerApplication&) = delete;
    TCPServerApplication& operator=(const TCPServerApplication&) = delete;

    int execute();
protected:
    void addSession(std::unique_ptr<TCPServerSession> newServerSession);
    void removeSession(TCPServerSession* serverSession);

    //Список активных сессий
    std::list<std::unique_ptr<TCPServerSession>> m_sessionList;

    //Хеш таблица для быстрого поиска по списку
    std::unordered_map<TCPServerSession*, std::list<std::unique_ptr<TCPServerSession>>::iterator> m_sessionHash;
};

#endif //TCP_SERVER_APPLICATION_H
