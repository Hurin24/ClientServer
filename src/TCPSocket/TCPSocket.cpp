#include "TCPSocket.h"

#include <fcntl.h>
#include <arpa/inet.h>
#include <cstring>

TCPSocket::TCPSocket()
{

}

TCPSocket::~TCPSocket()
{
    TCPSocket::close();
}

TCPSocket::TCPSocket(TCPSocket&& other) :
           m_socket(other.m_socket),
           m_ip(std::move(other.m_ip)),
           m_port(other.m_port),
           m_state(other.m_state),
           m_isNonBlocking(other.m_isNonBlocking),
           m_lastError(std::move(other.m_lastError))
{
    //Обнуляем исходный объект, чтобы он не закрыл сокет при своем уничтожении
    other.m_socket = -1;
    other.m_port = 0;
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
    m_ip = std::move(other.m_ip);
    m_port = other.m_port;
    m_state = other.m_state;
    m_isNonBlocking = other.m_isNonBlocking;
    m_lastError = std::move(other.m_lastError);

    //Обнуляем исходный объект
    other.m_socket = -1;
    other.m_port = 0;
    other.m_state = Error;
    other.m_isNonBlocking = false;

    return *this;
}

std::string TCPSocket::getIp() const
{
    return m_ip;
}

void TCPSocket::setIp(const std::string& ip)
{
    m_ip = ip;
}

int TCPSocket::getPort() const
{
    return m_port;
}

void TCPSocket::setPort(int port)
{
    m_port = port;
}

TCPSocket::TCPSocketState TCPSocket::getState() const
{
    return m_state;
}

bool TCPSocket::getIsNonBlocking() const
{
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
        setLastError("Не удалось получить флаги сокета, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

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
        setLastError("Не удалось установить флаги сокета, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось получить флаги
        return false;
    }


    //Устанваливаем новое значение флага isNonBlocking
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
    //Пробуем создать сокет
    m_socket = socket(AF_INET, SOCK_STREAM, 0);

    //Если не удалось создать сокет
    if(m_socket == -1)
    {
        //Не удалось создать сокет, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке создать сокет
        setLastError("Не удалось создать сокет, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось создать сокет
        return false;
    }



    //Пытаемся установить опцию SO_REUSEADDR для сокета
    int opt = 1;  // 1 = включить, 0 = выключить
    int result = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //Если не удалось установить опцию SO_REUSEADDR для сокета
    if(result == -1)
    {
        //Не удалось установить опцию SO_REUSEADDR для сокета, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке установить опцию SO_REUSEADDR для сокета
        setLastError("Не удалось установить опцию SO_REUSEADDR для сокета, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось установить опцию SO_REUSEADDR для сокета
        return false;
    }



    //Заполняем структуру sockaddr_in данными для привязки сокета к адресу
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // Обнуляем структуру
    address =
    {
        .sin_family = AF_INET,
        .sin_port = htons(m_port),
        .sin_addr.s_addr = inet_addr(m_ip.c_str())
    };


    //Пытаемся привязать сокет к адресу
    result = bind(m_socket, (struct sockaddr*)&address, sizeof(address));

    //Если не удалось привязать сокет к адресу
    if(result == -1)
    {
        //Не удалось привязать сокет к адресу, устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке привязать сокет к адресу
        setLastError("Не удалось привязать сокет к адресу, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

        //Возвращаем false, так как не удалось привязать сокет к адресу
        return false;
    }


    //Удалось создать сокет, устанавливаем состояние в Initialized
    setState(Initialized);

    //Возвращаем true, так как удалось создать сокет
    return true;
}

bool TCPSocket::connect(std::string ip, int port)
{
    //Устанавливаем состояние в Connecting
    setState(Connecting);


    //Заполняем структуру sockaddr_in данными для привязки подключения сокета к серверу
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // Обнуляем структуру
    address =
    {
        .sin_family = AF_INET,
        .sin_port = htons(m_port),
        .sin_addr.s_addr = inet_addr(m_ip.c_str())
    };


    //Пытаемся подключиться к серверу
    int result = ::connect(m_socket, (struct sockaddr*)&address, sizeof(address));

    //Если не удалось подключиться к серверу
    if(result == -1)
    {
        //Не удалось подключиться к серверу

        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку, произошедшую при попытке подключиться к серверу
        setLastError("Не удалось подключиться к серверу, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

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
        setLastError("Не удалось перевести сокет в режим прослушивания входящих соединений, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));

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
    int clientSocketDescriptor = ::accept(clientSocket.m_socket, (struct sockaddr*)&client_address, &client_addr_len);

    //Если не удалось принять соединение
    if(clientSocketDescriptor == -1)
    {
        //Не удалось принять соединение


        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            //Сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно

            //Это не является ошибкой
        }
        else
        {
            //Сокет не находится в неблокирующем режиме и операция отправки данных может быть выполнена немедленно

            //Устанавливаем состояние в Error
            setState(Error);

            //Записываем ошибку, произошедшую при отправке данных
            setLastError("Не удалось принять соединение, ошибка №" + std::to_string(errno) + ", расшифровка: " + std::string(strerror(errno)));
        }


        //Возвращаем false, так как не удалось принять соединение
        return false;
    }


    //Инициализируем переданный сокет для общения с клиентом
    clientSocket.m_socket = clientSocketDescriptor;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
    clientSocket.m_ip = std::string(client_ip);
    clientSocket.m_port = ntohs(client_address.sin_port);
    clientSocket.setState(Connected);


    //Возвращаем true, так как удалось принять соединение
    return true;
}

void TCPSocket::close()
{
    ::close(m_socket);
    m_state = Disconnected;
}

int TCPSocket::send(uint8_t* data, size_t size)
{
    while(true)
    {
        //Пытаемся отправить данные
        size_t result = ::send(m_socket, data, size, 0);

        //Если не удалось отправить данные
        if(result == -1)
        {
            //Не удалось отправить данные

            //Если сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                //Сокет находится в неблокирующем режиме и операция отправки данных не может быть выполнена немедленно

                //Это не является ошибкой
                //Пробуем отправить данные снова
                continue;
            }
            else
            {
                //Сокет не находится в неблокирующем режиме и операция отправки данных может быть выполнена немедленно

                //Устанавливаем состояние в Error
                setState(Error);

                //Записываем ошибку, произошедшую при отправке данных
                setLastError("Не удалось отправить данные, ошибка №" + std::to_string(errno) +
                            ", расшифровка: " + std::string(strerror(errno)));

                //Возвращаем -1, так как не удалось отправить данные
                return result;
            }
        }

        //Возвращаем результат
        return result;
    }
}

void TCPSocket::setState(TCPSocketState state)
{
    m_state = state;
}

void TCPSocket::setLastError(const std::string& errorMessage)
{
    m_lastError = std::move(errorMessage);
}
