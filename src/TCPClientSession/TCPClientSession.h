#ifndef TCP_CLIENT_SESSION_H
#define TCP_CLIENT_SESSION_H

#include "../ClientSession/ClientSession.h"
#include "../TCPSocket/TCPSocket.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>


class TCPClientRequest;
class TCPServerResponse;

class TCPClientSession : public ClientSession
{

public:
    TCPClientSession();
    ~TCPClientSession();

    TCPClientSession(const TCPClientSession&) = delete;
    TCPClientSession& operator=(const TCPClientSession&) = delete;

    TCPClientSession(TCPClientSession&& other)  = delete;
    TCPClientSession& operator=(TCPClientSession&& other) = delete;

    enum TCPClientSessionState
    {
        Disconnected,   //Сессия не подключена к серверу
        Connecting,     //Сессия в процессе подключения к серверу
        Connected,      //Сессия подключена к серверу
        Disconnecting,  //Сессия в процессе отключения от сервера
        Error           //При работе с сессией произошла ошибка
    };

    bool getIsWorking() const; //Получить флаг, указывающий на то, что поток сессии работает
    TCPClientSessionState getState() const;     //Получить состояние сессии

    std::string getServerIP() const;            //Получить IP-адрес сервера
    int getServerPort() const;                  //Получить порт сервера

    std::string getLastError() const;

    //Эта функция выполняет подключение к серверу
    bool connect(std::string serverIP, int serverPort);

    //Эта функция выполняет отключения от сервера
    bool disconnect();

    //Эта функция выполняет запрос к серверу
    bool sendRequest(TCPClientRequest&& clientRequest);


private:
    std::thread m_sessionThread;                    //Поток, в котором выполняется рабочий цикл сессии
    std::condition_variable m_sessionThreadConditionVariable;
    std::mutex m_sessionThreadConditionalVariableMutex;
    void sessionThread();                           //Функция, которая выполняется в отдельном потоке и в которой реализован рабочий цикл сессии


    bool m_isWorking = true;                        //Флаг, указывающий на то, что поток сессии работает
    void setIsWorking(bool newValue);               //Установить флаг указывающий на то, что поток сессии работает


    TCPClientSessionState m_state = Disconnected;   //Состояние сессии
    void setState(TCPClientSessionState newState);  //Установить состояние сессии


    std::string m_serverIP;                         //IP-адрес сервера
    int m_serverPort;                               //Порт сервера


    TCPSocket m_socket;                             //Сокет, связанный с сессией


    void receiveData(int socketDescriptor);
    void sendData();


    //Функции для работы
    void pushBack(TCPClientRequest&& clientRequest);
    std::list<TCPClientRequest>::iterator getFrontRequest();
    bool checkIsTimeoutExpired(std::list<TCPClientRequest>::iterator iterator);
    void removeRequest(std::list<TCPClientRequest>::iterator iterator);

    std::mutex m_requestQueueMutex;                 //Мьютекс для защиты очереди запросов на отправку
    std::list<TCPClientRequest> m_requestQueue;     //Очередь запросов на отправку

    std::vector<uint8_t> m_sendingData;             //Буфер для отправляемых данных
    ssize_t m_offsetSendingData;                    //Офсет для принимаемых данных

    std::vector<uint8_t> m_receivedData;            //Буфер для принимаемых данных
    ssize_t m_offsetReceivedData;                   //Офсет для принимаемых данных

    std::string m_lastError = "Нет ошибок";         //Последняя ошибка, произошедшая при работе сессии
    void setLastError(const std::string& errorMessage);
};

#endif //TCP_CLIENT_SESSION_H
