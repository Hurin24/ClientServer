#ifndef TCP_SERVER_SESSION_H
#define TCP_SERVER_SESSION_H

#include "../TCPSocket/TCPSocket.h"
#include "../TCPServerResponse/TCPServerResponse.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

class TCPServerApplication;
class TCPServerResponse;
class TCPClientRequest;

class TCPServerSession
{
public:
    TCPServerSession(TCPServerApplication* tcpServerApplication, TCPSocket&& clientSocket);
    ~TCPServerSession();

    TCPServerSession(const TCPServerSession&) = delete;
    TCPServerSession& operator=(const TCPServerSession&) = delete;

    TCPServerSession(TCPServerSession&& other);
    TCPServerSession& operator=(TCPServerSession&& other);

    enum TCPServerSessionState
    {
        Disconnected,   //Сессия не подключена к клиенту
        Connected,      //Сессия подключена к клиенту
        Disconnecting,  //Сессия в процессе отключения
        Error           //При работе с сессией произошла ошибка
    };

    bool getIsWorking() const;                  //Получить флаг, указывающий на то, что поток сессии работает
    TCPServerSessionState getState() const;     //Получить состояние сессии

    std::string getClientIP() const;            //Получить IP-адрес клиента
    int getClientPort() const;                  //Получить порт клиента

    std::string getLastError() const;

    //Эта функция выполняет отключение от клиента
    bool disconnect();

    //Эта функция отправляет ответ клиенту
    bool sendResponse(TCPServerResponse&& response);

private:
    TCPServerApplication* m_tcpServerApplication = nullptr;     //Указатель на TCPServerApplication

    std::thread m_sessionThread;                                //Поток, в котором выполняется рабочий цикл сессии
    std::condition_variable m_sessionThreadConditionVariable;
    std::mutex m_sessionThreadConditionalVariableMutex;
    void sessionThread();                                       //Функция, которая выполняется в отдельном потоке

    bool m_isWorking = true;                                    //Флаг, указывающий на то, что поток сессии работает
    void setIsWorking(bool newValue);                           //Установить флаг

    TCPServerSessionState m_state = Disconnected;               //Состояние сессии
    void setState(TCPServerSessionState newState);              //Установить состояние сессии

    std::string m_clientIP;                                     //IP-адрес клиента
    int m_clientPort = 0;                                       //Порт клиента

    TCPSocket m_socket;                                         //Сокет, связанный с сессией

    bool receiveData(int socketDescriptor);                     //Функция приема данных
    bool sendData(int socketDescriptor);                        //Функция отправки данных

    bool checkAndUpdateCurrentResponse();                       //Обновить текущий ответ, который отправляется клиенту

    std::mutex m_responseQueueMutex;                            //Мьютекс для защиты очереди ответов на отправку
    std::list<TCPServerResponse> m_responseQueue;               //Очередь ответов на отправку

    TCPServerResponse m_currentResponse;                        //Текущий ответ, который отправляется клиенту

    std::vector<uint8_t> m_sendingData = { 0 };                 //Буфер для отправляемых данных
    ssize_t m_offsetSendingData = 0;                            //Офсет для отправляемых данных

    std::vector<uint8_t> m_receivedData = { 0 };                //Буфер для принимаемых данных
    ssize_t m_offsetReceivedData = 0;                           //Офсет для принимаемых данных

    std::string m_lastError = "Нет ошибок";                     //Последняя ошибка, произошедшая при работе сессии
    void setLastError(const std::string& errorMessage);         //Функция записи последней ошибки
};

#endif //TCP_SERVER_SESSION_H
