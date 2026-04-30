#include "TCPClientSession.h"

#include <iostream>
#include <arpa/inet.h>

TCPClientSession::TCPClientSession() :
                  Session()
{

}

TCPClientSession::~TCPClientSession()
{

}

TCPClientSession::TCPClientSession(TCPClientSession&& other) :
                  Session(std::move(other)),
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

    Session::operator=(std::move(other));
    m_socket = std::move(other.m_socket);

    return *this;
}

bool TCPClientSession::sendRequest(TCPClientRequest&& tcpClientRequest)
{
    std::lock_guard<std::mutex> lockGuard(m_queueMutex);

    m_queue.push_back(std::move(tcpClientRequest));

    return true;
}

bool TCPClientSession::isHasRequest()
{
    std::lock_guard<std::mutex> lockGuard(m_queueMutex);

    return m_queue.size() > 0;
}