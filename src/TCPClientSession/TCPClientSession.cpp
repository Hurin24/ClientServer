#include "TCPClientSession.h"

#include "../TCPClientServerProtocol.h"
#include "../TCPClientRequest/TCPClientRequest.h"
#include "../TCPServerResponse/TCPServerResponse.h"

#include <iostream>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

TCPClientSession::TCPClientSession()
{
    m_currentRequest.setState(TCPClientRequest::TCPClientRequestState::Failed);

    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPClientSession::sessionThread, this);
}

TCPClientSession::~TCPClientSession()
{
    setIsWorking(false);

    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }
}

TCPClientSession::TCPClientSession(TCPClientSession&& other)
{
    //Запоминаем значение isWorking у other
    bool tempValue = other.getIsWorking();

    //Останавливаем поток у other
    other.setIsWorking(false);
    if(other.m_sessionThread.joinable())
    {
        other.m_sessionThread.join();
    }


    m_isWorking = tempValue;
    m_state = other.m_state;
    m_serverIP = std::move(other.m_serverIP);
    m_serverPort = other.m_serverPort;
    m_socket = std::move(other.m_socket);
    m_sendingData = std::move(other.m_sendingData);
    m_offsetSendingData = other.m_offsetSendingData;
    m_receivedData = std::move(other.m_receivedData);
    m_offsetReceivedData = other.m_offsetReceivedData;
    m_lastError = std::move(other.m_lastError);
    m_currentRequest = std::move(other.m_currentRequest);


    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPClientSession::sessionThread, this);


    //Обнуляем исходный объект
    other.m_state = Error;
    other.m_serverPort = 0;
    other.m_offsetSendingData = 0;
    other.m_offsetReceivedData = 0;
}

TCPClientSession& TCPClientSession::operator=(TCPClientSession&& other)
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


    //Останавливаем поток у this
    setIsWorking(false);
    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }


    m_isWorking = tempValue;
    m_state = other.m_state;
    m_serverIP = std::move(other.m_serverIP);
    m_serverPort = other.m_serverPort;
    m_socket = std::move(other.m_socket);
    m_sendingData = std::move(other.m_sendingData);
    m_offsetSendingData = other.m_offsetSendingData;
    m_receivedData = std::move(other.m_receivedData);
    m_offsetReceivedData = other.m_offsetReceivedData;
    m_lastError = std::move(other.m_lastError);
    m_currentRequest = std::move(other.m_currentRequest);


    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPClientSession::sessionThread, this);


     //Обнуляем исходный объект
    other.m_state = Error;
    other.m_serverPort = 0;
    other.m_offsetSendingData = 0;
    other.m_offsetReceivedData = 0;

    return *this;
}

bool TCPClientSession::getIsWorking() const
{
    return m_isWorking;
}

TCPClientSession::TCPClientSessionState TCPClientSession::getState() const
{
    return m_state;
}

std::string TCPClientSession::getServerIP() const
{
    return m_serverIP;
}

int TCPClientSession::getServerPort() const
{
    return m_serverPort;
}

std::string TCPClientSession::getLastError() const
{
    return m_lastError;
}

bool TCPClientSession::connect(std::string serverIP, int serverPort)
{
    //Устанавливаем состояние в Connecting
    setState(Connecting);

    //Пытаемся инициализировать сокет
    bool result = m_socket.initialize();

    //Если не удалось инициализировать сокет
    if(!result)
    {
        //Записываем ошибку, произошедшую при отправке данных
        setLastError(m_socket.getLastError());

        //Устанавливаем состояние в Error
        setState(Error);

        //Возвращаем false, так как не удалось инициализировать сокет
        return false;
    }

    //Пытаемся подключиться к серверу
    result = m_socket.connect(serverIP, serverPort);

    //Если не удалось подключиться к серверу
    if(!result)
    {
        //Записываем ошибку, произошедшую при подключении к серверу
        setLastError(m_socket.getLastError());

        //Устанавливаем состояние в Error
        setState(Error);

        //Возвращаем false, так как не удалось подключиться к сокету
        return false;
    }

    //Запоминаем IP-адрес и порт сервера
    m_serverIP = serverIP;
    m_serverPort = serverPort;

    //Устанавливаем состояние в Connected
    setState(Connected);

    //Возвращаем true, так как удалось подключиться к серверу
    return true;
}

bool TCPClientSession::disconnect()
{
    //Устанавливаем состояние в Disconnecting
    setState(Disconnecting);

    //Закрываем сокет
    m_socket.close();

    //Устанавливаем состояние в Disconnected
    setState(Disconnected);

    return true;
}

bool TCPClientSession::sendRequest(TCPClientRequest&& request)
{
    std::lock_guard<std::mutex> lock(m_requestQueueMutex);

    m_requestQueue.emplace_back(std::move(request));

    return true;
}

void TCPClientSession::sessionThread()
{
    using namespace TCPClientServerProtocol;

    //Таймаут для select (100 мс)
    const int selectTimeoutMs = 50;

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

        checkAndUpdateCurrentRequest();

        TCPClientRequest::TCPClientRequestState requestState = m_currentRequest.getState();
        switch(requestState)
        {
            case TCPClientRequest::Pending:
            case TCPClientRequest::Sending:
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

void TCPClientSession::setIsWorking(bool newValue)
{
    std::lock_guard<std::mutex> lockGuard(m_sessionThreadConditionalVariableMutex);

    if(m_isWorking != newValue)
    {
        m_isWorking = newValue;
        m_sessionThreadConditionVariable.notify_all();
    }
}

void TCPClientSession::setState(TCPClientSessionState newState)
{
    std::lock_guard<std::mutex> lockGuard(m_sessionThreadConditionalVariableMutex);

    if(m_state != newState)
    {
        m_state = newState;
        m_sessionThreadConditionVariable.notify_all();
    }
}

bool TCPClientSession::receiveData(int socketDescriptor)
{
    using namespace TCPClientServerProtocol;

    //Узнаём сколько байт ещё требуется считать
    ssize_t bytesAvailable = howMuchNeed(m_receivedData.data(), m_offsetReceivedData);

    //Если размера буфера не хватает для приема данных
    if(m_receivedData.size() < m_offsetReceivedData + bytesAvailable)
    {
        //Если размера буфера не хватает для приема данных
        m_receivedData.resize(m_offsetReceivedData + bytesAvailable);
    }

    //Считываем данные с учетом офсета
    int bytesReceived = m_socket.recv(m_receivedData.data() + m_offsetReceivedData, m_receivedData.size() - m_offsetReceivedData);

    //Если произошла ошибка
    if(bytesReceived == -1)
    {
        //Записываем ошибку, произошедшую при приеме данных
        setLastError(m_socket.getLastError());

        //Устанавливаем состояние в Error
        setState(Error);

        //Возвращаем false, так как произошла ошибка
        return false;
    }

    //Увеличиваем офсет
    m_offsetReceivedData += bytesReceived;

    bytesAvailable = howMuchNeed(m_receivedData.data(), m_offsetReceivedData);

    //Если достаточно байт для формирования пакета
    if(bytesAvailable <= 0)
    {
        //Достаточно байт для формирования пакета

        //Пытаемся извлечь все возможные ответы из полученных данных
        auto response = TCPServerResponse::deserialize(m_receivedData);

        //Если TCPServerResponse валиден
        if(response)
        {
            //Если ID полученного ответа от сервера равно ID текущего запроса
            if(m_currentRequest.getRequestID() == response->getResponseID())
            {
                //Изменяем статус текущего запроса
                m_currentRequest.setState(TCPClientRequest::ResponseReceived);

                const std::vector<uint8_t>& refData = response->getData();

                std::string tempString((const char*)refData.data(), refData.size());
                std::cout << tempString << std::endl;

                // Побайтовый вывод в десятичном формате
                std::cout << "Побайтовый вывод (dec): ";
                for(size_t i = 0; i < refData.size(); ++i) {
                    std::cout << (uint8_t)refData[i] << " ";
                }
                std::cout << std::endl;
            }
        }

        //Если офсет меньше 0
        if(bytesAvailable < 0)
        {
            //Офсет меньше 0
            //Это означает, что в данных уже есть полный пакет и возможно начало следующего пакета

            //Копируем их в начало буфера
            memcpy(m_receivedData.data(), m_receivedData.data() + m_offsetReceivedData, -bytesAvailable);

            //Устанавливаем новый офсет, который будет указывать на конец данных, которые уже есть в буфере
            m_offsetReceivedData = -bytesAvailable;
        }
        else
        {
            //Сбрасываем офсет
            m_offsetReceivedData = 0;
        }
    }

    //Возвращаем true, так как удалось считать данные
    return true;
}

bool TCPClientSession::sendData(int socketDescriptor)
{
    //Исходя из текущего состояния, предпринимаем следующие действия
    TCPClientRequest::TCPClientRequestState state = m_currentRequest.getState();

    switch(state)
    {
        case TCPClientRequest::Pending:
        {
            //Сериализуем запрос
            m_sendingData = std::move(m_currentRequest.serialize());

            //Сбрасываем offset
            m_offsetSendingData = 0;

            //Устанавливаем состояние Sending
            m_currentRequest.setState(TCPClientRequest::Sending);
        }
        case TCPClientRequest::Sending:
        {
            //Отправляем данные с учетом offset
            int bytesSent = m_socket.send(m_sendingData.data() + m_offsetSendingData, m_sendingData.size() - m_offsetSendingData);

            //Если произошла ошибка
            if(bytesSent == -1)
            {
                //Устанавливаем состояние Failed
                m_currentRequest.setState(TCPClientRequest::Failed);

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

                //Устанавливаем состояние WaitingResponse
                m_currentRequest.setState(TCPClientRequest::WaitingResponse);
                m_currentRequest.setSendTime(std::chrono::steady_clock::now());

                //Сбрасываем offset
                m_offsetSendingData = 0;

                //Очищаем буфер для отправки данных
                m_sendingData.clear();
            }
            break;
        }
        case TCPClientRequest::WaitingResponse:
        case TCPClientRequest::ResponseReceived:
        case TCPClientRequest::Failed:
        case TCPClientRequest::TimedOut:
        {
            //Ничего не делаем, эти состояния обрабатываются в checkAndUpdateCurrentRequest
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

bool TCPClientSession::checkAndUpdateCurrentRequest()
{
    bool wasChanged = false;

    TCPClientRequest::TCPClientRequestState state = m_currentRequest.getState();

    switch(state)
    {
        case TCPClientRequest::Pending:
        case TCPClientRequest::Sending:
            break;
        case TCPClientRequest::WaitingResponse:
        {
            //Проверяем, не вышло ли время
            if(m_currentRequest.isTimeoutExpired())
            {
                wasChanged = true;
            }
            break;
        }
        case TCPClientRequest::ResponseReceived:
        case TCPClientRequest::Failed:
        case TCPClientRequest::TimedOut:
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
        std::lock_guard<std::mutex> lock(m_requestQueueMutex);

        auto iterator = m_requestQueue.begin();

        if(iterator == m_requestQueue.end())
        {
            wasChanged = false;
        }
        else
        {
            m_currentRequest = std::move(*iterator);
            m_requestQueue.erase(iterator);
        }
    }


    return wasChanged;
}

void TCPClientSession::setLastError(const std::string& errorMessage)
{
    m_lastError = errorMessage;
}
