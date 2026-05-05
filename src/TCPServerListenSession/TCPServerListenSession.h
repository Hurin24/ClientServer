#ifndef TCP_SERVER_LISTEN_SESSION_H
#define TCP_SERVER_LISTEN_SESSION_H

#include "../ClientSession/ClientSession.h"
#include "../TCPSocket/TCPSocket.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <memory>

class TCPServerApplication;

class TCPServerListenSession : public ClientSession
{

public:
    TCPServerListenSession(TCPServerApplication* tcpServerApplication);
    ~TCPServerListenSession();

    TCPServerListenSession(const TCPServerListenSession&) = delete;
    TCPServerListenSession& operator=(const TCPServerListenSession&) = delete;

    TCPServerListenSession(TCPServerListenSession&& other);
    TCPServerListenSession& operator=(TCPServerListenSession&& other);

    enum TCPServerListenSessionState
    {
        Stopped,            //Сессия остановлена
        StartingListening,  //Сессия в процессе запуска
        Listening,          //Сессия слушает входящие подключения
        StoppingListening,  //Сессия в процессе остановки
        Error               //При работе с сессией произошла ошибка
    };

    bool getIsWorking() const;                  //Получить флаг, указывающий на то, что поток сессии работает
    TCPServerListenSessionState getState() const; //Получить состояние сессии

    std::string getListenIP() const;            //Получить IP-адрес для прослушивания
    int getListenPort() const;                  //Получить порт для прослушивания

    //Эта функция запускает прослушивание на указанном IP и порту
    bool startListen(std::string listenIP, int listenPort);

    //Эта функция останавливает прослушивание
    bool stopListen();


private:
    TCPServerApplication* m_tcpServerApplication = nullptr;

    std::thread m_sessionThread;                                //Поток, в котором выполняется рабочий цикл сессии
    std::condition_variable m_sessionThreadConditionVariable;
    std::mutex m_sessionThreadConditionalVariableMutex;
    void sessionThread();                                       //Функция, которая выполняется в отдельном потоке

    bool m_isWorking = true;                                    //Флаг, указывающий на то, что поток сессии работает
    void setIsWorking(bool newValue);                           //Установить флаг

    TCPServerListenSessionState m_state = Stopped;              //Состояние сессии
    void setState(TCPServerListenSessionState newState);        //Установить состояние сессии

    std::string m_listenIP;                                     //IP-адрес для прослушивания
    int m_listenPort = 0;                                       //Порт для прослушивания

    TCPSocket m_listenSocket;                                   //Слушающий сокет

    std::string m_lastError = "Нет ошибок";                     //Последняя ошибка, произошедшая при работе сессии
    void setLastError(const std::string& errorMessage);
};

#endif //TCP_SERVER_LISTEN_SESSION_H
