#include "TCPServerListenSession.h"

#include "../TCPServerApplication/TCPServerApplication.h"
#include "../TCPServerSession/TCPServerSession.h"

#include <iostream>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

TCPServerListenSession::TCPServerListenSession(TCPServerApplication* tcpServerApplication) :
                        m_tcpServerApplication(tcpServerApplication)
{
    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerListenSession::sessionThread, this);
}

TCPServerListenSession::~TCPServerListenSession()
{
    setIsWorking(false);

    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }
}

TCPServerListenSession::TCPServerListenSession(TCPServerListenSession&& other)
{
    //Запоминаем значение isWorking у other
    bool tempValue = other.getIsWorking();

    //Останавливаем поток у other
    other.setIsWorking(false);

    if(other.m_sessionThread.joinable())
    {
        other.m_sessionThread.join();
    }

    m_tcpServerApplication = other.m_tcpServerApplication;
    m_isWorking = tempValue;
    m_state = other.m_state;
    m_listenIP = other.m_listenIP;
    m_listenPort = other.m_listenPort;
    m_listenSocket = std::move(other.m_listenSocket);
    m_lastError = std::move(other.m_lastError);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerListenSession::sessionThread, this);

    //Обнуляем исходный объект
    other.m_tcpServerApplication = nullptr;
    other.m_state = Error;
    other.m_listenPort = 0;
}

TCPServerListenSession& TCPServerListenSession::operator=(TCPServerListenSession&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    //Запоминаем значение isWorking у other
    bool tempValue = other.getIsWorking();

    //Останавливаем поток у other
    other.setIsWorking(false);

    if(other.m_sessionThread.joinable())
    {
        other.m_sessionThread.join();
    }

    m_tcpServerApplication = other.m_tcpServerApplication;
    m_isWorking = tempValue;
    m_state = other.m_state;
    m_listenIP = other.m_listenIP;
    m_listenPort = other.m_listenPort;
    m_listenSocket = std::move(other.m_listenSocket);
    m_lastError = std::move(other.m_lastError);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerListenSession::sessionThread, this);

    //Обнуляем исходный объект
    other.m_tcpServerApplication = nullptr;
    other.m_state = Error;
    other.m_listenPort = 0;

    return *this;
}

bool TCPServerListenSession::getIsWorking() const
{
    return m_isWorking;
}

TCPServerListenSession::TCPServerListenSessionState TCPServerListenSession::getState() const
{
    return m_state;
}

std::string TCPServerListenSession::getListenIP() const
{
    return m_listenIP;
}

int TCPServerListenSession::getListenPort() const
{
    return m_listenPort;
}

std::string TCPServerListenSession::getLastError() const
{
    return m_lastError;
}

bool TCPServerListenSession::startListen(std::string listenIP, int listenPort)
{
    //Устанавливаем состояние в StartingListening
    setState(StartingListening);

    //Пытаемся инициализировать сокет
    bool result = m_listenSocket.initialize();

    if(!result)
    {
        setLastError("Не удалось инициализировать сокет. Ошибка: " + m_listenSocket.getLastError());
        setState(Error);
        return false;
    }

    //Пытаемся привязать сокет к IP и порту
    result = m_listenSocket.bind(listenIP, listenPort);

    if(!result)
    {
        setLastError("Не удалось привязать сокет. Ошибка: " + m_listenSocket.getLastError());
        setState(Error);
        return false;
    }

    //Пытаемся перевести сокет в режим прослушивания
    result = m_listenSocket.listen();

    if(!result)
    {
        setLastError("Не удалось перевести сокет в режим прослушивания. Ошибка: " + m_listenSocket.getLastError());
        setState(Error);
        return false;
    }

    //Запоминаем IP и порт
    m_listenIP = listenIP;
    m_listenPort = listenPort;

    //Устанавливаем состояние в Listening
    setState(Listening);

    return true;
}

void TCPServerListenSession::waitForStop()
{
    std::unique_lock<std::mutex> uniqueLock(m_stateMutex);

    m_stateConditionVariable.wait(uniqueLock, [this]() { return m_state == Stopped || m_state == Error; });
}

bool TCPServerListenSession::stopListen()
{
    //Устанавливаем состояние в StoppingListening
    setState(StoppingListening);

    //Закрываем слушающий сокет
    m_listenSocket.close();

    //Устанавливаем состояние в Stopped
    setState(Stopped);

    return true;
}

void TCPServerListenSession::sessionThread()
{
    //Таймаут для select (100 мс)
    const int selectTimeoutMs = 100;

    //Наборы файловых дескрипторов
    fd_set readSet;
    fd_set errorSet;

    while(m_isWorking)
    {
        if(getState() != Listening)
        {
            std::unique_lock<std::mutex> lock(m_sessionThreadConditionalVariableMutex);

            m_sessionThreadConditionVariable.wait(lock,
                [this]()
                {
                    return m_state == Listening || !m_isWorking;
                }
            );

            continue;
        }

        //Получаем дескриптор слушающего сокета
        int socketFd = m_listenSocket.getSocketDescriptor();

        //Если сокет не инициализирован
        if(socketFd == -1)
        {
            //Записываем ошибку
            setLastError("Сокет не инициализирован");

            //Устанавливаем состояние в Error
            setState(Error);

            //Переходим к следующей итерации цикла
            continue;
        }

        //Настраиваем таймаут для select
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = selectTimeoutMs * 1000;

        //Настраиваем наборы файловых дескрипторов
        FD_ZERO(&readSet);
        FD_ZERO(&errorSet);

        FD_SET(socketFd, &readSet);
        FD_SET(socketFd, &errorSet);

        //Вызываем select
        int selectResult = select(socketFd + 1, &readSet, nullptr, &errorSet, &timeout);

        //Проверяем ошибки select
        if(selectResult == -1)
        {
            if(errno != EINTR)
            {
                //Записываем ошибку
                setLastError("Произошла ошибка при работе с select: " + std::string(strerror(errno)));

                //Устанавливаем состояние в Error
                setState(Error);

                //Переходим к следующей итерации цикла
                continue;
            }
        }
        else if(selectResult > 0)
        {
            //Проверяем ошибки на слушающем сокете
            if(FD_ISSET(socketFd, &errorSet))
            {
                int errorCode;
                socklen_t errorCodeSize = sizeof(errorCode);
                getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &errorCode, &errorCodeSize);

                //Записываем ошибку
                setLastError("Ошибка на слушающем сокете: " + std::string(strerror(errorCode)));

                //Устанавливаем состояние в Error
                setState(Error);

                //Переходим к следующей итерации цикла
                continue;
            }

            //Обработка нового подключения
            if(FD_ISSET(socketFd, &readSet))
            {
                TCPSocket newClientSocket;

                if(m_listenSocket.accept(newClientSocket))
                {
                    //Создаем новую сессию для клиента
                    if(m_tcpServerApplication)
                    {
                        //Создаем новую сессию и перемещаем в нее сокет
                        std::unique_ptr<TCPServerSession> newTCPServerSession(new TCPServerSession(m_tcpServerApplication, std::move(newClientSocket)));

                        //Добавляем сессию в приложение
                        m_tcpServerApplication->addSession(std::move(newTCPServerSession));
                    }
                    else
                    {
                        //Записываем ошибку
                        setLastError("Указатель на TCPServerApplication равен nullptr");

                        //Устанавливаем состояние в Error
                        setState(Error);
                    }
                }
                else
                {
                    //Записываем ошибку
                    setLastError("Ошибка при принятии подключения: " + m_listenSocket.getLastError());

                    //Не устанавливаем состояние Error, так как это не критично для слушающей сессии
                    //Просто логируем ошибку и продолжаем работу
                }
            }
        }
    }
}

void TCPServerListenSession::setIsWorking(bool newValue)
{
    std::lock_guard<std::mutex> lockGuard(m_sessionThreadConditionalVariableMutex);

    if(m_isWorking != newValue)
    {
        m_isWorking = newValue;
        m_sessionThreadConditionVariable.notify_all();
    }
}

void TCPServerListenSession::setState(TCPServerListenSessionState newState)
{
    std::lock_guard<std::mutex> lockGuard1(m_stateMutex);
    std::lock_guard<std::mutex> lockGuard2(m_sessionThreadConditionalVariableMutex);

    if(m_state != newState)
    {
        m_state = newState;
        m_stateConditionVariable.notify_all();
        m_sessionThreadConditionVariable.notify_all();
    }
}

void TCPServerListenSession::setLastError(const std::string& errorMessage)
{
    m_lastError = errorMessage;
}