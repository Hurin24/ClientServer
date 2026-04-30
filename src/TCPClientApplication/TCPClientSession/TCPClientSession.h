#ifndef TCP_CLIENT_SESSION_H
#define TCP_CLIENT_SESSION_H

#include "../../Session/Session.h"
#include "../../TCPSocket/TCPSocket.h"

#include <condition_variable>
#include <list>

class TCPClientRequest;
class TCPServerResponse;

class TCPClientSession : public Session
{

public:
    TCPClientSession();
    ~TCPClientSession();

    TCPClientSession(const TCPClientSession&) = delete;
    TCPClientSession& operator=(const TCPClientSession&) = delete;

    TCPClientSession(TCPClientSession&& other);
    TCPClientSession& operator=(TCPClientSession&& other);


    bool connect(std::string serverIp, int serverPort);

    bool sendRequest(TCPClientRequest&& tcpClientRequest);

    bool isHasRequest();


protected:
    void onResponseRecieved(const TCPServerResponse& tcpServerResponse) override;    //Эта функция принимает и парсит ответ от сервера


private:
    TCPSocket m_socket;             //Сокет, связанный с сессией
    std::string m_serverIp;         //IP-адрес сервера
    int m_serverPort;               //Порт сервера

    std::mutex m_queueMutex;                //Мьютекс для защиты очереди запросов на отправку
    std::list<TCPClientRequest> m_queue;    //Очередь запросов на отправку
};

#endif //TCP_CLIENT_SESSION_H
