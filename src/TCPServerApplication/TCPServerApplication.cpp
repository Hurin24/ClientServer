#include "TCPServerApplication.h"

#include "../TCPClientRequest/TCPClientRequest.h"
#include "../TCPServerListenSession/TCPServerListenSession.h"
#include "../TCPServerSession/TCPServerSession.h"

#include <utility>
#include <iostream>
#include <arpa/inet.h>

TCPServerApplication::TCPServerApplication()
{

}

TCPServerApplication::~TCPServerApplication()
{

}

int TCPServerApplication::execute()
{
    std::string serverIp;
    int serverPort;

    std::cout << "Введите IPv4 адрес на котором вы хотите запустить сервер: ";
    std::cin >> serverIp;
    std::cout << std::endl;

    struct sockaddr_in sa_ipv4;

    if(inet_pton(AF_INET, serverIp.c_str(), &sa_ipv4.sin_addr) != 1)
    {
        //Выводим ошибку, произошедшую при попытке инициализировать сокет
        std::cout << "Ошибка: введён не валидный IPv4 адрес" << std::endl;

        //Завершаем выполнение функции, так как был введён не валидный IPv4 адрес
        return 1;
    }


    std::cout << "Введите порт на котором хотите запустить сервер: ";
    std::cin >> serverPort;
    std::cout << std::endl;


    //Создаём сессию
    TCPServerListenSession tcpServerListenSession(this);

    //Производим попытку начать слушать сокет
    bool result = tcpServerListenSession.startListen(serverIp,  serverPort);

    //Если не удалось начать слушать сокет
    if(!result)
    {
        //Не удалось начать слушать сокет

        //Выводим ошибку
        std::cout << tcpServerListenSession.getLastError() << std::endl;

        //Возвращаем результат работы приложения
        return 1;
    }


    //Удалось начать слушать сокет

    //Ожидаем того момента когда сессия перестанет слушать
    tcpServerListenSession.waitForStop();

    //Если завершили с состоянием Error
    if(tcpServerListenSession.getState() == TCPServerListenSession::TCPServerListenSessionState::Error)
    {
        //Выводим ошибку
        std::cout << tcpServerListenSession.getLastError() << std::endl;

        //Возвращаем результат работы приложения
        return 1;
    }

    //Возвращаем результат работы приложения
    return 0;
}

void TCPServerApplication::addSession(std::unique_ptr<TCPServerSession> newServerSession)
{
    if(!newServerSession)
    {
        return;
    }

    //Получаем сырой указатель для хеш-таблицы
    TCPServerSession* rawPtr = newServerSession.get();

    //Добавляем в список
    auto iterator = m_sessionList.insert(m_sessionList.end(), std::move(newServerSession));

    //Итератор на последний элемент
    m_sessionHash[rawPtr] = iterator;
}

void TCPServerApplication::removeSession(TCPServerSession* serverSession)
{
    if(!serverSession)
    {
        return;
    }

    //Ищем сессию в хеш-таблице
    auto hashIt = m_sessionHash.find(serverSession);

    if(hashIt != m_sessionHash.end())
    {
        // Получаем итератор из списка
        auto listIt = hashIt->second;

        //Удаляем из списка
        m_sessionList.erase(listIt);

        //Удаляем из хеш-таблицы
        m_sessionHash.erase(hashIt);
    }
}