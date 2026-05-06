#ifndef TCP_CLIENT_SESSION_H
#define TCP_CLIENT_SESSION_H

#include "../TCPSocket/TCPSocket.h"
#include "../TCPClientRequest/TCPClientRequest.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

class TCPClientSession
{

public:
    TCPClientSession();
    ~TCPClientSession();

    TCPClientSession(const TCPClientSession&) = delete;
    TCPClientSession& operator=(const TCPClientSession&) = delete;

    TCPClientSession(TCPClientSession&& other);
    TCPClientSession& operator=(TCPClientSession&& other);

    enum TCPClientSessionState
    {
        Disconnected,   //Сессия не подключена к серверу
        Connecting,     //Сессия в процессе подключения к серверу
        Connected,      //Сессия подключена к серверу
        Disconnecting,  //Сессия в процессе отключения от сервера
        Error           //При работе с сессией произошла ошибка
    };

    bool getIsWorking() const;                  //Получить флаг, указывающий на то, что поток сессии работает
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
    std::thread m_sessionThread;                                //Поток, в котором выполняется рабочий цикл сессии
    std::condition_variable m_sessionThreadConditionVariable;
    std::mutex m_sessionThreadConditionalVariableMutex;
    void sessionThread();                                       //Функция, которая выполняется в отдельном потоке

    bool m_isWorking = true;                                    //Флаг, указывающий на то, что поток сессии работает
    void setIsWorking(bool newValue);                           //Установить флаг

    TCPClientSessionState m_state = Disconnected;               //Состояние сессии
    void setState(TCPClientSessionState newState);              //Установить состояние сессии

    std::string m_serverIP;                                     //IP-адрес сервера
    int m_serverPort = 0;                                       //Порт сервера

    TCPSocket m_socket;                                         //Сокет, связанный с сессией

    bool receiveData(int socketDescriptor);                     //Функция приема данных
    bool sendData(int socketDescriptor);                        //Функция отправки данных

    bool checkAndUpdateCurrentRequest();                        //Обновить текущий запрос, который отправляется серверу

    std::mutex m_requestQueueMutex;                             //Мьютекс для защиты очереди запросов на отправку
    std::list<TCPClientRequest> m_requestQueue;                 //Очередь запросов на отправку

    TCPClientRequest m_currentRequest;                          //Текущий запрос, который отправляется серверу

    std::vector<uint8_t> m_sendingData = { 0 };                 //Буфер для отправляемых данных
    ssize_t m_offsetSendingData = 0;                            //Офсет для отправляемых данных

    std::vector<uint8_t> m_receivedData = { 0 };                //Буфер для принимаемых данных
    ssize_t m_offsetReceivedData = 0;                           //Офсет для принимаемых данных

    std::string m_lastError = "Нет ошибок";                     //Последняя ошибка, произошедшая при работе сессии
    void setLastError(const std::string& errorMessage);         //Функция записи последней ошибки
};

#endif //TCP_CLIENT_SESSION_H
