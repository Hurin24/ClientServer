#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <cstdint>

class TCPSocket
{

public:
    TCPSocket();
    ~TCPSocket();

    TCPSocket(const TCPSocket&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;

    TCPSocket(TCPSocket&& other);
    TCPSocket& operator=(TCPSocket&& other);

    //Список состояний сокета
    enum TCPSocketState
    {
        NotInitialized, //Сокет не инициализирован
        Initialized,    //Сокет инициализирован, но не подключен
        Connecting,     //Сокет в процессе подключения
        Connected,      //Сокет подключен
        Listening,      //Сокет находится в режиме прослушивания входящих соединений
        Disconnected,   //Сокет не подключен
        Error           //При работе с сокетом произошла ошибка
    };


    std::string getIp() const;
    void setIp(const std::string& ip);

    int getPort() const;
    void setPort(int port);

    TCPSocketState getState() const;

    bool getIsNonBlocking() const;
    bool setIsNonBlocking(bool isNonBlocking);

    std::string getLastError() const;



    //Эта функция инициализирует сокет
    bool initialize();

    //Эта функция подключается к серверу по IP-адресу и порту
    bool connect(std::string ip, int port);

    //Эта функция переводит сокет в режим прослушивания входящих соединений
    bool listen();

    //Эта функция принимает входящее соединение на слушающем сокете и инициализирует переданный сокет для общения с клиентом
    bool accept(TCPSocket& clientSocket);

    //Эта функция закрывает соединение с удалённой стороной
    void close();

    //Эта отправляет данные данные удалённой стороне
    int send(uint8_t* data, size_t size);

    //Эта принимает данные от удалённой стороны
    int recv(uint8_t* data, size_t size);


private:
    int m_socket = -1;  //Дескриптор сокета
    std::string m_ip;   //IP-адрес сервера
    int m_port;         //Порт сервера
    TCPSocketState m_state = NotInitialized;  //Состояние сокета
    bool m_isNonBlocking = false;             //Флаг, указывающий на то, что сокет находится в неблокирующем режиме
    std::string m_lastError = "Нет ошибок";   //Последняя ошибка, произошедшая при работе с сокетом

    void setState(TCPSocketState state);
    void setLastError(const std::string& errorMessage);
};

#endif //TCP_SOCKET_H
