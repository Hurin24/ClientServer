#include "TCPSocket.h"

#include <fcntl.h>
#include <arpa/inet.h>
#include <cstring>

TCPSocket::TCPSocket()
{
    initialize();
}

TCPSocket::~TCPSocket()
{
    TCPSocket::close();
}

TCPSocket::TCPSocket(TCPSocket&& other) :
           m_socket(other.m_socket),
           m_localIp(other.m_localIp),
           m_localPort(other.m_localPort),
           m_remoteIp(std::move(other.m_remoteIp)),
           m_remotePort(other.m_remotePort),
           m_state(other.m_state),
           m_isNonBlocking(other.m_isNonBlocking),
           m_lastError(std::move(other.m_lastError))
{
    //Обнуляем исходный объект
    other.m_socket = -1;
    other.m_localPort = 0;
    other.m_remotePort = 0;
    other.m_state = Error;
    other.m_isNonBlocking = false;
}

TCPSocket& TCPSocket::operator=(TCPSocket&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    //Перемещаем данные из other в this
    m_socket = other.m_socket;
    m_localIp = std::move(other.m_localIp);
    m_localPort = other.m_localPort;
    m_remoteIp = std::move(other.m_remoteIp);
    m_remotePort = other.m_remotePort;
    m_state = other.m_state;
    m_isNonBlocking = other.m_isNonBlocking;
    m_lastError = std::move(other.m_lastError);

    //Обнуляем исходный объект
    other.m_socket = -1;
    other.m_localPort = 0;
    other.m_remotePort = 0;
    other.m_state = Error;
    other.m_isNonBlocking = false;

    return *this;
}

std::string TCPSocket::getLocalIp() const
{
    return m_localIp;
}

int TCPSocket::getLocalPort() const
{
    return m_localPort;
}

std::string TCPSocket::getRemoteIp() const
{
    return m_remoteIp;
}

int TCPSocket::getRemotePort() const
{
    return m_remotePort;
}

TCPSocket::TCPSocketState TCPSocket::getState() const
{
    return m_state;
}

bool TCPSocket::getIsNonBlocking()
{
    //Пытаемся получить текущие флаги сокета
    int flags = fcntl(m_socket, F_GETFL, 0);

    //Если не удалось получить флаги
    if(flags == -1)
    {
        //Не удалось получить флаги

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке получить флаги сокета
        setLastError("Не удалось получить флаги сокета, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось получить флаги
        return false;
    }

    m_isNonBlocking = flags & O_NONBLOCK;

    return m_isNonBlocking;
}

bool TCPSocket::setIsNonBlocking(bool isNonBlocking)
{
    //Пытаемся получить текущие флаги сокета
    int flags = fcntl(m_socket, F_GETFL, 0);

    //Если не удалось получить флаги
    if(flags == -1)
    {
        //Не удалось получить флаги

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке получить флаги сокета
        setLastError("Не удалось получить флаги сокета, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось получить флаги
        return false;
    }


    //Устанавливаем флаги исходя из переданного isNonBlocking
    if(isNonBlocking)
    {
        flags |= O_NONBLOCK;
    }
    else
    {
        flags &= ~O_NONBLOCK;
    }


    //Пытаемся установить новые флаги сокета
    int result = fcntl(m_socket, F_SETFL, flags);

    //Если не удалось установить флаги
    if(result == -1)
    {
        //Не удалось установить флаги

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке установить флаги сокета
        setLastError("Не удалось установить флаги сокета, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось получить флаги
        return false;
    }


    //Устанавливаем новое значение флага isNonBlocking
    m_isNonBlocking = isNonBlocking;


    //Возвращаем true, так как удалось получить флаг
    return true;
}

std::string TCPSocket::getLastError() const
{
    return m_lastError;
}

bool TCPSocket::initialize()
{
    //Если сокет был инициализирован
    if(m_socket != -1)
    {
        //Cокет был инициализирован

        //Закрываем
        close();
    }


    std::unique_lock<std::mutex> unique_lock(m_socketMutex);

    //Пробуем создать сокет
    m_socket = socket(AF_INET, SOCK_STREAM, 0);

    //Если не удалось создать сокет
    if(m_socket == -1)
    {
        //Не удалось создать сокет, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке создать сокет
        setLastError("Не удалось создать сокет, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось создать сокет
        return false;
    }

    unique_lock.unlock();


    //Пытаемся установить опцию SO_REUSEADDR для сокета
    int opt = 1;  // 1 = включить, 0 = выключить
    int result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //Если не удалось установить опцию SO_REUSEADDR для сокета
    if(result == -1)
    {
        //Не удалось установить опцию SO_REUSEADDR для сокета, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке установить опцию SO_REUSEADDR для сокета
        setLastError("Не удалось установить опцию SO_REUSEADDR для сокета, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось установить опцию SO_REUSEADDR для сокета
        return false;
    }


    //Удалось создать сокет, устанавливаем состояние в Initialized
    setState(Initialized);

    //Возвращаем true, так как удалось создать сокет
    return true;
}

bool TCPSocket::bind(std::string ip, int port)
{
    //Заполняем структуру sockaddr_in данными для привязки сокета к адресу
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); //Обнуляем структуру
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if(ip.size() < 0)
    {
        address.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        address.sin_addr.s_addr = inet_addr(ip.c_str());
    }

    //Пытаемся привязать сокет к адресу
    bool result = ::bind(m_socket, (struct sockaddr*)&address, sizeof(address));

    //Если не удалось привязать сокет к адресу
    if(result == -1)
    {
        //Не удалось привязать сокет к адресу, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке привязать сокет к адресу
        setLastError("Не удалось привязать сокет к адресу, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось привязать сокет к адресу
        return false;
    }


    m_localPort = port;

    if(ip.size() < 0)
    {
        m_localIp = "0.0.0.0";
    }

    //Возвращаем true, так как удалось привязать сокет к адресу
    return true;
}

bool TCPSocket::connect(std::string ip, int port)
{
    //Устанавливаем состояние в Connecting
    setState(Connecting);


    //Заполняем структуру sockaddr_in данными для привязки подключения сокета к серверу
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // Обнуляем структуру
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip.c_str());


    //Пытаемся подключиться к серверу
    int result = ::connect(m_socket, (struct sockaddr*)&address, sizeof(address));

    //Если не удалось подключиться к серверу
    if(result == -1)
    {
        //Не удалось подключиться к серверу

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке подключиться к серверу
        setLastError("Не удалось подключиться к серверу, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось подключиться к серверу
        return false;
    }



    //Устанавливаем состояние в Connected
    setState(Connected);


    //Возвращаем true, так как удалось подключиться к серверу
    return true;
}

bool TCPSocket::listen()
{
    //Пытаемся перевести сокет в режим прослушивания входящих соединений
    int result = ::listen(m_socket, 5);

    //Если не удалось перевести сокет в режим прослушивания входящих соединений
    if(result == -1)
    {
        //Не удалось перевести сокет в режим прослушивания входящих соединений

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке перевести сокет в режим прослушивания входящих соединений
        setLastError("Не удалось перевести сокет в режим прослушивания входящих соединений, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось перевести сокет в режим прослушивания входящих соединений
        return false;
    }

    //Устанавливаем состояние в Listening
    setState(Listening);

    //Возвращаем true, так как удалось перевести сокет в режим прослушивания входящих соединений
    return true;
}

bool TCPSocket::accept(TCPSocket& clientSocket)
{
    //Структура для хранения адреса клиента
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address)); //Обнуляем структуру
    socklen_t client_addr_len = sizeof(client_address);

    //Пытаемся принять входящее соединение
    int clientSocketDescriptor = ::accept(m_socket, (struct sockaddr*)&client_address, &client_addr_len);

    //Если не удалось принять соединение
    if(clientSocketDescriptor == -1)
    {
        //Не удалось принять соединение

        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            //Сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно

            //Это не является ошибкой
            return false;
        }
        else
        {
            //Сокет не находится в неблокирующем режиме и операция отправки данных может быть выполнена немедленно

            //Устанавливаем состояние в Error
            setState(Error);

            //Записываем ошибку, произошедшую при отправке данных
            setLastError("Не удалось принять соединение, ошибка № " + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));
        }


        //Возвращаем false, так как не удалось принять соединение
        return false;
    }


    //Инициализируем переданный сокет для общения с клиентом
    clientSocket.m_socket = clientSocketDescriptor;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
    clientSocket.m_localIp = std::string(client_ip);
    clientSocket.m_localPort = ntohs(client_address.sin_port);
    clientSocket.setState(Connected);


    //Возвращаем true, так как удалось принять соединение
    return true;
}

void TCPSocket::close()
{
    ::close(m_socket);
    reset();
}

ssize_t TCPSocket::send(uint8_t* data, ssize_t size)
{
    ssize_t totalSent = 0;

    while(true)
    {
        //Пытаемся отправить данные
        ssize_t result = ::send(m_socket, data + totalSent, size - totalSent, 0);

        //Если не удалось отправить данные
        if(result == -1)
        {
            //Если сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return totalSent;
            }
            else
            {
                //Произошла ошибка
                setState(Error);
                setLastError("Не удалось отправить данные, ошибка № " + std::to_string(errno) +
                            ", расшифровка: " + std::string(strerror(errno)));

                return result;
            }
        }
        else if(result == 0)
        {
            return totalSent;
        }
        else
        {
            totalSent += result;

            if(totalSent == size)
            {
                return totalSent;
            }
        }
    }

    return totalSent;
}

#include <iostream>

ssize_t TCPSocket::recv(uint8_t* data, ssize_t size)
{
    ssize_t totalReceived = 0;

    while(true)
    {
        //Пытаемся принять данные
        ssize_t result = ::recv(m_socket, data + totalReceived, size - totalReceived, 0);

        //Если не удалось принять данные
        if(result == -1)
        {
            //Если сокет находится в неблокирующем режиме и операция приема данных не может быть выполнена немедленно
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                //Нечего считывать, выходим
                break;
            }
            else
            {
                //Произошла ошибка
                setLastError("Не удалось принять данные, ошибка № " + std::to_string(errno) +
                            ", расшифровка: " + std::string(strerror(errno)));
                setState(Error);

                return -1;
            }
        }
        else
        {
            totalReceived += result;
            break;
        }
    }

    return totalReceived;
}

int TCPSocket::getSocketDescriptor()
{
    std::lock_guard<std::mutex> lockGuard(m_socketMutex);

    return m_socket;
}

void TCPSocket::reset()
{
    std::lock_guard<std::mutex> lockGuard(m_socketMutex);

    setState(NotInitialized);

    m_socket = -1;

    std::string m_localIp = "";
    m_localPort= -1;

    std::string m_remoteIp = "";
    m_remotePort= -1;
}

void TCPSocket::setState(TCPSocketState state)
{
    m_state = state;
}

void TCPSocket::setLastError(const std::string& errorMessage)
{
    m_lastError = std::move(errorMessage);
}
