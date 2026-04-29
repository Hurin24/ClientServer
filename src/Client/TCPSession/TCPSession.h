#ifndef TCP_SESSION_H
#define TCP_SESSION_H

#include "../../Session/Session.h"

#include "../../TCPSocket/TCPSocket.h"

class TCPSession : public Session
{

public:
    TCPSession(std::string serverIp, int serverPort);
    ~TCPSession();

    TCPSession(const TCPSession&) = delete;
    TCPSession& operator=(const TCPSession&) = delete;

    TCPSession(TCPSession&& other);
    TCPSession& operator=(TCPSession&& other);


protected:
    bool execute() override;        //Рабочий цикл сессии, выполняется в отдельном потоке


private:
    TCPSocket m_socket;             //Сокет, связанный с сессией
    std::string m_serverIp;         //IP-адрес сервера
    int m_serverPort;               //Порт сервера
};

#endif //TCP_SESSION_H
