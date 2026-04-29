#include "TCPSession.h"
#include <iostream>
#include <arpa/inet.h>

TCPSession::TCPSession(std::string serverIp, int serverPort) :
            m_serverIp(serverIp),
            m_serverPort(serverPort)
{

}

TCPSession::~TCPSession()
{

}

TCPSession::TCPSession(TCPSession&& other) :
            Session(std::move(other)),
            m_socket(std::move(other.m_socket))
{

}

TCPSession& TCPSession::operator=(TCPSession&& other)
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

bool TCPSession::execute()
{
    std::cout << "Введите IPv4 адрес сервера, к которому хотите подключиться: ";
    std::cin >> m_serverIp;
    std::cout << std::endl;

    struct sockaddr_in sa_ipv4;

    if(inet_pton(AF_INET, m_serverIp.c_str(), &sa_ipv4.sin_addr) == 1)
    {
        //Выводим ошибку, произошедшую при попытке инициализировать сокет
        std::cout << "Ошибка: введён не валидный IPv4 адрес" << std::endl;

        //Завершаем выполнение функции, так как был введён не валидный IPv4 адрес
        return false;
    }


    std::cout << "Введите порт сервера, к которому хотите подключиться: ";
    std::cin >> m_serverPort;
    std::cout << std::endl;

    m_socket.setIp(m_serverIp);
    m_socket.setPort(m_serverPort);

    //Пробуем инициализировать сокет
    bool result = m_socket.initialize();

    //Если не удалось инициализировать сокет
    if(!result)
    {
        //Не удалось инициализировать сокет

        //Выводим ошибку, произошедшую при попытке инициализировать сокет
        std::cout << "Не удалось инициализировать сокет, ошибка: " << m_socket.getLastError() << std::endl;

        //Завершаем выполнение функции, так как не удалось инициализировать сокет
        return result;
    }

    //Пробуем подключиться к серверу
    result = m_socket.connect(m_serverIp, m_serverPort);

    //Если не удалось подключиться к серверу
    if(!result)
    {
        //Не удалось подключиться к серверу

        //Выводим ошибку, произошедшую при попытке подключиться к серверу
        std::cout << "Не удалось подключиться к серверу, ошибка: " << m_socket.getLastError() << std::endl;

        //Завершаем выполнение функции, так как не удалось подключиться к серверу
        return result;
    }


    std::cout << "Вводите символы которые хотите отправить на сервер:" << std::endl;

    std::string buffer;

    //Пока сессия в работе
    while(m_isWorking)
    {
        //Считываем символы, которые хотим отправить на сервер
        std::cin >> buffer;

        //Пытаемся отправить данные на сервер
        //Пока не отправим все данные
        size_t bytesSend = 0;
        while(bytesSend < buffer.size())
        {
            //Отправляем данные на сервер
            int result = m_socket.send(reinterpret_cast<uint8_t*>(buffer.data()) + bytesSend, buffer.size() - bytesSend);

            if(result == -1)
            {
                //Не удалось отправить данные

                //Выводим ошибку, произошедшую при попытке отправить данные
                std::cout << "Не удалось отправить данные, ошибка: " << m_socket.getLastError() << std::endl;

                if(errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    //Сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно

                    //Это не является ошибкой
                    //Пробуем отправить данные снова
                    continue;
                }

                //Завершаем выполнение функции, так как не удалось отправить данные
                return false;
            }

            bytesSend += result;
        }


        result = m_socket.send(buffer.data(), buffer.size());
    }
}
