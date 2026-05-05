#include "TCPClientRequest.h"
#include "../TCPClientServerProtocol.h"

#include <chrono>
#include <cstring>
#include <arpa/inet.h>

//Инициализация статического атомарного счетчика
std::atomic<uint64_t> TCPClientRequest::m_nextRequestID {1};  // Начинаем с 1

TCPClientRequest::TCPClientRequest() :
                  m_state(Pending),
                  m_requestID(0),
                  m_timeoutMs(5000)
{

}

TCPClientRequest::~TCPClientRequest()
{

}

TCPClientRequest::TCPClientRequest(TCPClientRequest&& other) :
                  m_data(std::move(other.m_data)),
                  m_state(other.m_state),
                  m_requestID(other.m_requestID),
                  m_sendTime(other.m_sendTime),
                  m_timeoutMs(other.m_timeoutMs)
{
    //Обнуляем исходный объект
    other.m_state = Pending;
    other.m_requestID = 0;
    other.m_timeoutMs = 5000;
}

TCPClientRequest& TCPClientRequest::operator=(TCPClientRequest&& other)
{
    if(this != &other)
    {
        m_data = std::move(other.m_data);
        m_state = other.m_state;
        m_requestID = other.m_requestID;
        m_sendTime = other.m_sendTime;
        m_timeoutMs = other.m_timeoutMs;

        //Обнуляем исходный объект
        other.m_state = Pending;
        other.m_requestID = 0;
        other.m_timeoutMs = 5000;
    }

    return *this;
}

size_t TCPClientRequest::generateNewRequestID()
{
    //Генерируем новый ID для запроса
    return m_nextRequestID.fetch_add(1, std::memory_order_relaxed);
}

TCPClientRequest::TCPClientRequestState TCPClientRequest::getState() const
{
    return m_state;
}

const std::vector<uint8_t>& TCPClientRequest::getData() const
{
    return m_data;
}

uint32_t TCPClientRequest::getRequestID() const
{
    return m_requestID;
}

std::chrono::steady_clock::time_point TCPClientRequest::getSendTime() const
{
    return m_sendTime;
}

int TCPClientRequest::getTimeout() const
{
    return m_timeoutMs;
}

void TCPClientRequest::setState(TCPClientRequestState state)
{
    m_state = state;
}

void TCPClientRequest::setData(const std::vector<uint8_t>& data)
{
    m_data = data;
}

void TCPClientRequest::setData(std::vector<uint8_t>&& data)
{
    m_data = std::move(data);
}

void TCPClientRequest::setRequestID(size_t ID)
{
    m_requestID = ID;
}

void TCPClientRequest::setSendTime(std::chrono::steady_clock::time_point time)
{
    m_sendTime = time;
}

void TCPClientRequest::setTimeout(int timeoutMs)
{
    //Проверяем корректность таймаута
    if(timeoutMs > 0)
    {
        m_timeoutMs = timeoutMs;
    }
    else
    {
        m_timeoutMs = 5000; // Значение по умолчанию при некорректном вводе
    }
}

bool TCPClientRequest::isTimeoutExpired() const
{
    //Таймаут проверяется только для запросов в состоянии ожидания ответа
    if(m_state != WaitingResponse)
    {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sendTime);

    return elapsed.count() >= m_timeoutMs;
}

std::vector<uint8_t> TCPClientRequest::serialize() const
{
    using namespace TCPClientServerProtocol;

    std::vector<uint8_t> result;

    //Добавляем поле данных
    ssize_t dataSizeNet = REQUEST_HEADER_SIZE + m_data.size();
    const uint8_t* dataSizeBytes = reinterpret_cast<const uint8_t*>(&dataSizeNet);
    result.insert(result.end(), dataSizeBytes, dataSizeBytes + sizeof(dataSizeNet));

    //Добавляем ID запроса
    size_t requestIdNet = m_requestID;
    const uint8_t* requestIdBytes = reinterpret_cast<const uint8_t*>(&requestIdNet);
    result.insert(result.end(), requestIdBytes, requestIdBytes + sizeof(requestIdNet));

    //Добавляем данные
    if(!m_data.empty())
    {
        result.insert(result.end(), m_data.begin(), m_data.end());
    }

    return result;
}

std::shared_ptr<TCPClientRequest> TCPClientRequest::deserialize(std::vector<uint8_t>& data)
{
    using namespace TCPClientServerProtocol;

    //Если данных для заголовка недостаточно
    if(data.size() < sizeof(RequestHeader))
    {
        return nullptr;
    }

    //Кастуем начало данных к RequestHeader
    RequestHeader* header = reinterpret_cast<RequestHeader*>(data.data());

    //Если данных для полного пакета недостаточно
    if(data.size() < header->dataSize)
    {
        return nullptr;
    }

    //Создаем объект запроса
    auto request = std::shared_ptr<TCPClientRequest>(new TCPClientRequest);
    request->m_requestID = header->id;

    //Копируем данные со смещением(без заголовка)
    if(header->dataSize > 0)
    {
        int tempSize = header->dataSize - sizeof(RequestHeader);

        request->m_data.resize(tempSize);
        std::copy(data.begin() + sizeof(RequestHeader), data.begin() + tempSize, request->m_data.begin());
    }

    return request;
}