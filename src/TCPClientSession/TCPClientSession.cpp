#include "TCPClientSession.h"

#include <iostream>
#include <arpa/inet.h>

TCPClientSession::TCPClientSession()
{
    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPClientSession::sessionThread, this);
}

TCPClientSession::~TCPClientSession()
{
    m_isWorking = false;

    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }
}

TCPClientSessionState TCPClientSession::getState() const
{
    return m_state;
}

std::string TCPClientSession::getServerIP() const
{
    return m_serverIP;
}

int TCPClientSession::getServerPort() const
{
    return m_serverPort;
}

bool TCPClientSession::connect(std::string serverIP, int serverPort)
{
    //Устанавливаем состояние в Connecting
    setState(Connecting);


    //Пытаемся инициализировать сокет
    bool result = m_socket.initialize();

    //Если не удалось инициализировать сокет
    if(!result)
    {
        //Не удалось инициализировать сокет
        //Записываем ошибку, произошедшую при отправке данных
        setLastError("Не удалось инициализировать сокет. Ошибка: " + m_socket.getLastError());

        //Возвращаем false, так как не удалось инициализировать сокет
        return false;
    }


    //Пытаемся подключиться к серверу
    result = m_socket.connect(serverIP, serverPort);

    //Если не удалось подключиться к серверу
    if(!result)
    {
        //Не удалось подключиться к серверу

        //Записываем ошибку, произошедшую при подключении к серверу
        setLastError("Не удалось подключиться к серверу. Ошибка: " + m_socket.getLastError());

        //Возвращаем false, так как не удалось подключиться к сокету
        return false;
    }


    //Запоминаем IP-адрес и порт сервера
    m_serverIP = serverIP;
    m_serverPort = serverPort;


    //Устанавливаем состояние в Connected
    setState(Connected);


    //Возвращаем true, так как удалось подключиться к серверу
    return true;
}

bool TCPClientSession::disconnect()
{
    //Устанавливаем состояние в Disconnecting
    setState(Disconnecting);

    //Закрываем сокет
    m_socket.close();

    //Устанавливаем состояние в Disconnected
    setState(Disconnected);

    return true;
}

bool TCPClientSession::sendRequest(TCPClientRequest&& request)
{
    std::lock_guard<std::mutex> lockGuard(m_requestQueueMutex);

    m_requestQueue.emplace_back(std::move(request));

    return true;
}

void TCPClientSession::sessionThread()
{
    //Пока сессия в работе
    while(m_isWorking)
    {

    }
}

void TCPClientSession::setState(TCPClientSessionState state)
{
    m_state = state;
}
