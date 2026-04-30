#include "TCPClientSession.h"

#include <iostream>
#include <arpa/inet.h>

TCPClientSession::TCPClientSession() :
                  ClientSession()
{

}

TCPClientSession::~TCPClientSession()
{

}

TCPClientSession::TCPClientSession(TCPClientSession&& other) :
                  ClientSession(std::move(other)),
                  m_socket(std::move(other.m_socket))
{

}

TCPClientSession& TCPClientSession::operator=(TCPClientSession&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    ClientSession::operator=(std::move(other));
    m_socket = std::move(other.m_socket);

    return *this;
}

bool TCPClientSession::connect(std::string serverIp, int serverPort)
{
    m_socket.initialize();
}

bool TCPClientSession::disconnect()
{
    return false;
}

bool TCPClientSession::sendRequest(ClientRequest&& request)
{
    //Попытаться привести к нужному типу
    TCPClientRequest* tcpClientRequest = dynamic_cast<TCPClientRequest*>(&request);

    if(!tcpClientRequest)
    {
        //Не тот тип
        //Ошибка
        return false;
    }

    std::lock_guard<std::mutex> lockGuard(m_queueMutex);

    m_queue.push_back(std::move(tcpClientRequest));

    return true;
}

bool TCPClientSession::isHasRequest()
{
    std::lock_guard<std::mutex> lockGuard(m_queueMutex);

    return m_queue.size() > 0;
}