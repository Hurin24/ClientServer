#include "TCPClientSession.h"

#include <TCPClientServerProtocol>

#include <iostream>
#include <arpa/inet.h>

TCPClientSession::TCPClientSession()
{
    //Запускаем рабочий цикл сессии в отдельном потоке
    m_sessionThread = std::thread(&TCPClientSession::sessionThread, this);
}

TCPClientSession::~TCPClientSession()
{
    setIsWorking(false)

    if(m_sessionThread.joinable())
    {
        m_sessionThread.join();
    }
}

TCPClientSessionState TCPClientSession::getIsWorking() const
{
    return m_isWorking;
}

TCPClientSessionState TCPClientSession::getState() const
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

bool TCPClientSession::connect(std::string serverIP, int serverPort)
{
    //Устанавливаем состояние в Connecting
    setState(Connecting);


    //Пытаемся инициализировать сокет
    bool result = m_socket.initialize();

    //Если не удалось инициализировать сокет
    if(!result)
    {
        //Не удалось инициализировать сокет
        //Записываем ошибку, произошедшую при отправке данных
        setLastError("Не удалось инициализировать сокет. Ошибка: " + m_socket.getLastError());

        //Возвращаем false, так как не удалось инициализировать сокет
        return false;
    }


    //Пытаемся подключиться к серверу
    result = m_socket.connect(serverIP, serverPort);

    //Если не удалось подключиться к серверу
    if(!result)
    {
        //Не удалось подключиться к серверу

        //Записываем ошибку, произошедшую при подключении к серверу
        setLastError("Не удалось подключиться к серверу. Ошибка: " + m_socket.getLastError());

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
    std::lock_guard<std::mutex> lockGuard(m_requestQueueMutex);

    m_requestQueue.emplace_back(std::move(request));

    return true;
}

void TCPClientSession::sessionThread()
{
    using namespace TCPClientServerProtocol;

    //Буфер для приема данных
    std::vector<uint8_t> receivedData(sizeof(ResponseHeader))

    //Буфер для отправки данных
    std::vector<uint8_t> sendingData_;

    //Оболочка для более удобного высчитывания оффсета
    std::span<uint8_t> sendingData_

    //Таймаут для select (100 мс)
    const int selectTimeoutMs = 100;

    ////Наборы файловых дескрипторов
    fd_set readSet;
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
            setLastError("Сокет не инициализирован");
            setState(Error);
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


        //Вызываем select для проверки на чтение
        int selectResult = select(socketFd + 1, &readSet, nullptr, &errorSet, &timeout);

        //Проверяем ошибки select
        if(selectResult == -1)
        {
            if(errno != EINTR)
            {
                setLastError("Ошибка в select: " + std::string(strerror(errno)));
                setState(Error);
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

                setLastError("Ошибка в сокете: " + std::string(strerror(errorCode)));
                setState(Error);
                continue;
            }


            //Обработка приема данных
            if(FD_ISSET(socketFd, &readSet))
            {
                //Переменная хранящая размер байт, которые планируется считать
                int bytesAvailable = 0;
                ioctl(socketFd, FIONREAD, &bytesAvailable);

                if(bytesAvailable >= 0)
                {
                    //Если доступных данных больше чем размер буфера, увеличиваем буфер

                    //Переменная хранящая реальный размер буфера в который считываем данные
                    size_t freeSizeReceiveBuffer = receiveBuffer.capacity() - receiveBuffer.size();

                    //Если свободное место в буфере меньше чем количество байт которое планируется считать
                    if(static_cast<size_t>(bytesAvailable) > freeSizeReceiveBuffer)
                    {
                        //Увеличиваем размер свободного места в буфере
                        receiveBuffer.reserve(receiveBuffer.size() + bytesAvailable);
                    }

                    //Считываем данные
                    int bytesReceived = m_socket.recv(receiveBuffer.data() + receiveBuffer.size(), receiveBuffer.capacity());

                    //Если удалось что то считать
                    if(bytesReceived > 0)
                    {
                        //Обрезаем буфер до реально полученных данных
                        receiveBuffer.resize(receiveBuffer.size() + bytesReceived);

                        //Если достаточно байт для формирования пакета
                        if(howMuchNeed(receiveBuffer) == 0)
                        {
                            //Достаточно байт для формирования пакета

                            //Пытаемся извлечь все возможные ответы из полученных данных
                            auto response = TCPServerResponce::deserialize(receiveBuffer);

                            //Если TCPServerResponce валиден
                            if(response)
                            {
                                std::lock_guard<std::mutex> lock_guard(m_requestQueueMutex);

                                //Ищем соответствующий запрос в очереди ожидающих
                                auto it = pendingRequests.begin();

                                if(it->getRequestID() == response->getRequestID())
                                {
                                    //ToDo добавить функцию для обработки полученного ответа

                                    //Удаляем запрос, который получил ответ
                                    it = pendingRequests.erase(it);
                                }
                            }
                        }
                    }
                    else
                    {
                        setLastError("Ошибка при приеме данных: " + m_socket.getLastError());
                        setState(Error);
                    }
                }
            }
        }

        //Проверяем таймаут ожидающего запроса
        checkTimeoutRequestQueue();
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

void TCPClientSession::receiveData()
{
    //Если достигли конца буфера
    if(m_receivedData.size() == m_offsetReceivedData)
    {
        //Достигли конца буфера

        //Узнаём сколько байт ожидается считать
        ssize_t bytesAvailable = 0;
        ioctl(socketFd, FIONREAD, &bytesAvailable);

        if(bytesAvailable > 0)
        {
            //Увеличиваем размер свободного места в буфере
            receiveBuffer.resize(receiveBuffer.size() + howMuchNeed(receiveBuffer));
        }

        //To Do тут нужно придумать ветку развития событий
    }


    //Считываем данные
    int bytesReceived = m_socket.recv(receiveBuffer.data() + m_offsetReceivedData, howMuchNeed(receiveBuffer));

    //Если произошла ошибка
    if(bytesReceived == -1)
    {
        setLastError("Ошибка при приеме данных: " + m_socket.getLastError());
        setState(Error);
    }
    else
    {
        //Увеличиваем офсет
        m_offsetReceivedData += bytesReceived;

        //Если достаточно байт для формирования пакета
        if(howMuchNeed(receiveBuffer) == 0)
        {
            //Достаточно байт для формирования пакета

            //Пытаемся извлечь все возможные ответы из полученных данных
            auto response = TCPServerResponce::deserialize(receiveBuffer);

            //Если TCPServerResponce валиден
            if(response)
            {
                //Получем итератор на первый в очереди запрос
                std::list<TCPClientRequest>::iterator iterator = getFrontRequest();

                //Если он равен концу списка
                if(iterator == m_requestQueue.end())
                {
                    //Он равен концу списка

                    //Выходим
                    return;
                }

                //Если ID полученного ответа от сервера равно ID первому в очереди запросу
                if(iterator->getRequestID() == response->getRequestID())
                {
                    //To Do нужно добавить функцию

                    //Изменяем статус
                    iterator->setState(TCPClientRequest::ResponseReceived);
                }
            }
        }
    }
}

void TCPClientSession::sendData()
{
    //Получем итератор на первый в очереди запрос
    std::list<TCPClientRequest>::iterator iterator = getFrontRequest();

    //Если он равен концу списка
    if(iterator == m_requestQueue.end())
    {
        //Он равен концу списка

        //Выходим, нечего отправлять
        return;
    }


    //Исходя из текущего состояния, предпринимаем следующие действия
    TCPClientRequest::TCPClientRequestState state = iterator->getState();

    switch(state)
    {
        case TCPClientRequest::Pending:
        {
            //Сериализуем запрос

            //Запоминаем данные для отправки
            m_sendingData = std::move(iterator->serialize());

            //Сбрасываем offset
            m_offsetSendingData = 0;

            //Устанавливаем состояние Sending
            request.setState(TCPClientRequest::Sending);
        }
        case TCPClientRequest::Sending:
        {
            //Отправляем данные с учетом offset
            int bytesSent = m_socket.send(m_sendingData.data() + m_offsetSendingData, m_sendingData.size() - m_offsetSendingData);

            //Если произошла ошибка
            if(bytesSent == -1)
            {
                //Помечаем что запрос провалился
                request.setState(TCPClientRequest::Failed);

                //Удаляем запрос
                removeRequest(iterator);

                //Устанавливаем новое состояние сессии
                setState(Error);

                //Выходим
                break;
            }
            else
            {
                //Смещаем offset вправо на количество отправленных байт
                m_offsetSendingData += bytesSent;
            }


            //Если больше нечего отправлять
            if(m_offsetSendingData == m_sendingData.size())
            {
                iterator->setState(TCPClientRequest::WaitingResponse);
                iterator->setSendTime(std::chrono::steady_clock::now());
                m_offsetSendingData = 0;
            }
            break;
        }
        case TCPClientRequest::WaitingResponse:
        {
            //Проверяем, не вышло ли время
            if(checkIsTimeoutExpired(iterator))
            {
                //Время вышло

                //Удаляем запрос
                removeRequest(iterator);
            }
            break;
        }
        case TCPClientRequest::ResponseReceived:
        case TCPClientRequest::Failed:
        case TCPClientRequest::TimedOut:
        {
            removeRequest(iterator);
        }
        default:
        {
            removeRequest(iterator);
            break;
        }
    }
}

void TCPClientSession::pushBack(TCPClientRequest&& clientRequest)
{
    m_requestQueue.emplace_back(std::move(clientRequest));
}

std::list<TCPClientRequest>::iterator TCPClientSession::getFrontRequest()
{
    return m_requestQueue.begin();
}

bool TCPClientSession::checkIsTimeoutExpired(std::list<TCPClientRequest>::iterator iterator)
{
    if(iterator == m_requestQueue.end())
    {
        return false;
    }

    if(iterator->getState() != WaitingResponse)
    {
        return false;
    }

    return iterator->isTimeoutExpired();
}

void TCPClientSession::removeRequest(std::list<TCPClientRequest>::iterator iterator)
{
    if(iterator == m_requestQueue.end())
    {
        return;
    }

    m_requestQueue.erase(iterator);
}

