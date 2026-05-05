// TCPServerSession.cpp
#include "TCPServerSession.h"

#include "../TCPClientServerProtocol.h"
#include "../TCPServerApplication/TCPServerApplication.h"
#include "../TCPClientRequest/TCPClientRequest.h"

#include <iostream>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

TCPServerSession::TCPServerSession(TCPServerApplication* tcpServerApplication, TCPSocket&& clientSocket) :
                  m_tcpServerApplication(tcpServerApplication),
                  m_socket(std::move(clientSocket))
{
    //Получаем IP и порт клиента
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    if(getpeername(m_socket.getSocketDescriptor(), (struct sockaddr*)&clientAddr, &addrLen) == 0)
    {
        m_clientIP = inet_ntoa(clientAddr.sin_addr);
        m_clientPort = ntohs(clientAddr.sin_port);
    }

    m_currentResponse.setState(TCPServerResponse::Failed);

    if(m_socket.getState() != TCPSocket::Connected)
    {
        //Устанавливаем состояние в Error
        setState(Error);

        //Записываем ошибку
        setLastError("Сокет не находится в состоянии Connected");

        return;
    }

    //Устанавливаем состояние в Connected
    setState(Connected);

    //Устанавливаем состояние в Failed
    m_currentResponse.setState(TCPServerResponse::Failed);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerSession::sessionThread, this);
}

TCPServerSession::~TCPServerSession()
{
    setIsWorking(false);

    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }
}

TCPServerSession::TCPServerSession(TCPServerSession&& other)
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
    m_clientIP = other.m_clientIP;
    m_clientPort = other.m_clientPort;
    m_socket = std::move(other.m_socket);
    m_sendingData = std::move(other.m_sendingData);
    m_offsetSendingData = other.m_offsetSendingData;
    m_receivedData = std::move(other.m_receivedData);
    m_offsetReceivedData = other.m_offsetReceivedData;
    m_lastError = std::move(other.m_lastError);

    m_currentResponse.setState(TCPServerResponse::Failed);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerSession::sessionThread, this);

    //Обнуляем исходный объект
    other.m_tcpServerApplication = nullptr;
    other.m_state = Error;
    other.m_clientPort = -1;
}

TCPServerSession& TCPServerSession::operator=(TCPServerSession&& other)
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
    m_clientIP = other.m_clientIP;
    m_clientPort = other.m_clientPort;
    m_socket = std::move(other.m_socket);
    m_sendingData = std::move(other.m_sendingData);
    m_offsetSendingData = other.m_offsetSendingData;
    m_receivedData = std::move(other.m_receivedData);
    m_offsetReceivedData = other.m_offsetReceivedData;
    m_lastError = std::move(other.m_lastError);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPServerSession::sessionThread, this);

    //Обнуляем исходный объект
    other.m_tcpServerApplication = nullptr;
    other.m_state = Error;
    other.m_clientPort = -1;

    return *this;
}

bool TCPServerSession::getIsWorking() const
{
    return m_isWorking;
}

TCPServerSession::TCPServerSessionState TCPServerSession::getState() const
{
    return m_state;
}

std::string TCPServerSession::getClientIP() const
{
    return m_clientIP;
}

int TCPServerSession::getClientPort() const
{
    return m_clientPort;
}

std::string TCPServerSession::getLastError() const
{
    return m_lastError;
}

bool TCPServerSession::disconnect()
{
    //Устанавливаем состояние в Disconnecting
    setState(Disconnecting);

    //Закрываем сокет
    m_socket.close();

    //Устанавливаем состояние в Disconnected
    setState(Disconnected);

    return true;
}

bool TCPServerSession::sendResponse(TCPServerResponse&& response)
{
    std::lock_guard<std::mutex> lock(m_responseQueueMutex);

    m_responseQueue.emplace_back(std::move(response));

    return true;
}

void TCPServerSession::sessionThread()
{
    using namespace TCPClientServerProtocol;

    //Таймаут для select (100 мс)
    const int selectTimeoutMs = 100;

    //Наборы файловых дескрипторов
    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;

    while(m_isWorking)
    {
        if(getState() != Connected)
        {
            std::unique_lock<std::mutex> lock(m_sessionThreadConditionalVariableMutex);

            m_sessionThreadConditionVariable.wait(lock,
                [this]()
                {
                    return m_state == Connected || !m_isWorking;
                }
            );

            continue;
        }

        //Получаем дескриптор сокета
        int socketFd = m_socket.getSocketDescriptor();

        //Если сокет не инициализирован
        if(socketFd == -1)
        {
            //Записываем ошибку
            setLastError("Сокет не инициализирован");

            //Устанавливаем состояние в Error
            setState(Error);

            //Переходим к следующей итерации цикла, так как произошла ошибка
            continue;
        }

        //Настраиваем таймаут для select
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = selectTimeoutMs * 1000;


        //Настраиваем наборы файловых дескрипторов
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_ZERO(&errorSet);


        FD_SET(socketFd, &readSet);
        FD_SET(socketFd, &errorSet);


        checkAndUpdateCurrentResponse();

        TCPServerResponse::TCPServerResponseState responseState = m_currentResponse.getState();
        switch(responseState)
        {
            case TCPServerResponse::TCPServerResponseState::Pending:
            case TCPServerResponse::TCPServerResponseState::Sending:
                FD_SET(socketFd, &writeSet);
                break;
            default:
                break;
        }


        //Вызываем select
        int selectResult = select(socketFd + 1, &readSet, &writeSet, &errorSet, &timeout);

        //Проверяем ошибки select
        if(selectResult == -1)
        {
            if(errno != EINTR)
            {
                //Записываем ошибку
                setLastError("Произошла ошибка при работе с select: " + std::string(strerror(errno)));

                //Устанавливаем состояние в Error
                setState(Error);

                //Переходим к следующей итерации цикла, так как произошла ошибка
                continue;
            }
        }
        else if(selectResult > 0)
        {
            //Проверяем наличие ошибок на сокете
            if(FD_ISSET(socketFd, &errorSet))
            {
                int errorCode;
                socklen_t errorCodeSize = sizeof(errorCode);
                getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &errorCode, &errorCodeSize);

                //Записываем ошибку
                setLastError("В сокете произошла ошибка: " + std::string(strerror(errorCode)));

                //Устанавливаем состояние в Error
                setState(Error);

                //Переходим к следующей итерации цикла, так как произошла ошибка
                continue;
            }

            //Обработка приема данных
            if(FD_ISSET(socketFd, &readSet))
            {
                bool isOk = receiveData(socketFd);

                if(!isOk)
                {
                    //Произошла ошибка при приеме данных, она уже обработана внутри receiveData

                    //Переходим к следующей итерации цикла, так как произошла ошибка
                    continue;
                }
            }

            //Обработка отправки данных
            if(FD_ISSET(socketFd, &writeSet))
            {
                bool isOk = sendData(socketFd);

                if(!isOk)
                {
                    //Произошла ошибка при отправке данных, она уже обработана внутри sendData

                    //Переходим к следующей итерации цикла, так как произошла ошибка
                    continue;
                }
            }
        }
    }
}

void TCPServerSession::setIsWorking(bool newValue)
{
    std::lock_guard<std::mutex> lockGuard(m_sessionThreadConditionalVariableMutex);

    if(m_isWorking != newValue)
    {
        m_isWorking = newValue;
        m_sessionThreadConditionVariable.notify_all();
    }
}

void TCPServerSession::setState(TCPServerSessionState newState)
{
    std::lock_guard<std::mutex> lockGuard(m_sessionThreadConditionalVariableMutex);

    if(m_state != newState)
    {
        m_state = newState;
        m_sessionThreadConditionVariable.notify_all();
    }
}

bool TCPServerSession::receiveData(int socketDescriptor)
{
    using namespace TCPClientServerProtocol;

    //Узнаём сколько байт ещё требуется считать
    ssize_t bytesAvailable = howMuchNeed(m_receivedData);

    //Если достигли конца буфера
    if(m_receivedData.size() - m_offsetReceivedData < bytesAvailable)
    {
        //Увеличиваем размер буфера для приема данных
        m_receivedData.resize(m_receivedData.size() + bytesAvailable);
    }

    //Считываем данные с учетом офсета
    int bytesReceived = m_socket.recv(m_receivedData.data() + m_offsetReceivedData, bytesAvailable);

    //Если произошла ошибка
    if(bytesReceived == -1)
    {
        //Записываем ошибку, произошедшую при приеме данных
        setLastError("Не удалось считать данные. Ошибка: " + m_socket.getLastError());

        //Устанавливаем состояние в Error
        setState(Error);

        //Возвращаем false, так как произошла ошибка
        return false;
    }

    //Увеличиваем офсет
    m_offsetReceivedData += bytesReceived;

    std::cout << "before: ";

    for(int i = 0; i < m_receivedData.size(); i++)
    {
        uint8_t newUint8_t = *(reinterpret_cast<uint8_t*>(m_receivedData.data() + i));
        int newInt = newUint8_t;
        std::cout << newInt;
    }

    std::cout << std::endl;

    //Увеличиваем размер буфера исходя из количества принятых байт
    m_receivedData.resize(m_receivedData.size() + bytesReceived);

    std::cout << "after: ";

    for(int i = 0; i < m_receivedData.size(); i++)
    {
        uint8_t newUint8_t = *(reinterpret_cast<uint8_t*>(m_receivedData.data() + i));
        int newInt = newUint8_t;
        std::cout << newInt;
    }

    std::cout << std::endl;


    //Если достаточно байт для формирования пакета
    if(howMuchNeed(m_receivedData) == 0)
    {
        //Достаточно байт для формирования пакета

        //Пытаемся извлечь все возможные запросы из полученных данных
        auto request = TCPClientRequest::deserialize(m_receivedData);

        //Если TCPClientRequest валиден
        if(request)
        {
            //Отправляем обратно
            TCPServerResponse tcpServerResponse(std::move(*request));
            sendResponse(std::move(tcpServerResponse));

            //Сбрасываем офсет
            m_offsetReceivedData = 0;

            //Сбрасываем размер буфера, так как все данные уже обработаны
            m_receivedData.resize(0);
        }
    }

    //Возвращаем false, так как удалось считать данные
    return true;
}

bool TCPServerSession::sendData(int socketDescriptor)
{
    //Исходя из текущего состояния, предпринимаем следующие действия
    TCPServerResponse::TCPServerResponseState state = m_currentResponse.getState();

    switch(state)
    {
        case TCPServerResponse::Pending:
        {
            //Сериализуем ответ
            m_sendingData = std::move(m_currentResponse.serialize());

            //Сбрасываем offset
            m_offsetSendingData = 0;

            //Устанавливаем состояние Sending
            m_currentResponse.setState(TCPServerResponse::Sending);
        }
        case TCPServerResponse::Sending:
        {
            //Отправляем данные с учетом offset
            int bytesSent = m_socket.send(m_sendingData.data() + m_offsetSendingData, m_sendingData.size() - m_offsetSendingData);

            //Если произошла ошибка
            if(bytesSent == -1)
            {
                //Устанавливаем состояние Sending
                m_currentResponse.setState(TCPServerResponse::Failed);

                //Устанавливаем новое состояние сессии
                setState(Error);

                //Возвращаем false, так как не удалось отправить данные
                return false;
            }
            else
            {
                //Смещаем offset вправо на количество отправленных байт
                m_offsetSendingData += bytesSent;
            }

            //Если больше нечего отправлять
            if(m_offsetSendingData == m_sendingData.size())
            {
                //Больше нечего отправлять

                //Устанавливаем состояние WasSended
                m_currentResponse.setState(TCPServerResponse::WasSended);

                //Сбрасываем offset
                m_offsetSendingData = 0;

                //Очищаем буфер для отправки данных
                m_sendingData.clear();
            }
            break;
        }
        case TCPServerResponse::WasSended:
        case TCPServerResponse::Failed:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    //Возвращаем true, так как отправка данных прошла успешно (даже если внутри произошла ошибка, мы её обработали и не нужно останавливать сессию)
    return true;
}


bool TCPServerSession::checkAndUpdateCurrentResponse()
{
    bool wasChanged = false;

    TCPServerResponse::TCPServerResponseState state = m_currentResponse.getState();

    switch(state)
    {
        case TCPServerResponse::Pending:
        case TCPServerResponse::Sending:
        {
            break;
        }
        case TCPServerResponse::WasSended:
        case TCPServerResponse::Failed:
        {
            wasChanged = true;
            break;
        }
        default:
        {
            break;
        }
    }


    if(wasChanged)
    {
        std::lock_guard<std::mutex> lock(m_responseQueueMutex);

        auto iterator = m_responseQueue.begin();

        if(iterator == m_responseQueue.end())
        {
            wasChanged = false;
        }
        else
        {
            m_currentResponse = std::move(*iterator);
            m_responseQueue.erase(iterator);
        }
    }


    return wasChanged;
}

void TCPServerSession::setLastError(const std::string& errorMessage)
{
    m_lastError = errorMessage;
}
