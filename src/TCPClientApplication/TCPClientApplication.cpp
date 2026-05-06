#include "TCPClientApplication.h"

#include "../TCPClientRequest/TCPClientRequest.h"
#include "../TCPClientSession/TCPClientSession.h"

#include <utility>
#include <iostream>
#include <arpa/inet.h>

TCPClientApplication::TCPClientApplication()
{

}

TCPClientApplication::~TCPClientApplication()
{

}

int TCPClientApplication::execute()
{
    std::string serverIp;
    int serverPort;

    std::cout << "Введите IPv4 адрес сервера, к которому хотите подключиться: ";
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


    std::cout << "Введите порт сервера, к которому хотите подключиться: ";
    std::cin >> serverPort;
    std::cout << std::endl;


    //Создаём сессию
    TCPClientSession tcpClientSession;

    // Производим попытку подключения к серверу
    bool result = tcpClientSession.connect(serverIp,  serverPort);
    // bool result = tcpClientSession.connect("127.0.0.1", 49999);

    //Если не удалось подключиться к серверу
    if(!result)
    {
        //Не удалось подключиться к серверу

        //Выводим ошибку
        std::cout << tcpClientSession.getLastError() << std::endl;

        //Возвращаем результат работы приложения
        return 1;
    }

    std::cout << "Вводите символы которые хотите отправить на сервер:" << std::endl;

    std::string buffer;

    //Пока сессия в работе
    while(true)
    {
        //Считываем символы, которые хотим отправить на сервер
        std::cin >> buffer;

        //Если состояние сессии не Connected
        if(tcpClientSession.getState() != TCPClientSession::TCPClientSessionState::Connected)
        {
            //Состояние сессии не Connected

            //Выводим ошибку
            std::cout << tcpClientSession.getLastError() << std::endl;

            //Возвращаем результат работы приложения
            return 1;
        }

        std::vector<uint8_t> vec(buffer.begin(), buffer.end());

        //Формируем запрос
        TCPClientRequest newTCPClientRequest;
        newTCPClientRequest.setData(std::move(vec));

        //Отправляем запрос
        tcpClientSession.sendRequest(std::move(newTCPClientRequest));
    }

    return 0;
}
