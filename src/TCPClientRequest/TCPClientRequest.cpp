#include "TCPClientRequest.h"
#include <chrono>
#include <cstring>
#include <arpa/inet.h>

//Инициализация статического атомарного счетчика
std::atomic<uint64_t> TCPClientRequest::m_nextID{1};  // Начинаем с 1

TCPClientRequest::TCPClientRequest()
    : m_state(Pending)
    , m_requestId(0)
    , m_timeoutMs(5000)
{
    //Автоматически генерируем новый ID
    m_requestId = m_nextID.fetch_add(1, std::memory_order_relaxed);
}

TCPClientRequest::~TCPClientRequest()
{
}

TCPClientRequest::TCPClientRequest(TCPClientRequest&& other)
    : m_data(std::move(other.m_data))
    , m_state(other.m_state)
    , m_requestId(other.m_requestId)
    , m_sendTime(other.m_sendTime)
    , m_timeoutMs(other.m_timeoutMs)
{
    // Обнуляем исходный объект
    other.m_state = Pending;
    other.m_requestId = 0;
    other.m_timeoutMs = 5000;
}

TCPClientRequest& TCPClientRequest::operator=(TCPClientRequest&& other)
{
    if (this != &other)
    {
        m_data = std::move(other.m_data);
        m_state = other.m_state;
        m_requestId = other.m_requestId;
        m_sendTime = other.m_sendTime;
        m_timeoutMs = other.m_timeoutMs;

        //Обнуляем исходный объект
        other.m_state = Pending;
        other.m_requestId = 0;
        other.m_timeoutMs = 5000;
    }

    return *this;
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
    return m_requestId;
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

void TCPClientRequest::setRequestID(uint32_t ID)
{
    m_requestId = ID;
}

void TCPClientRequest::setSendTime(std::chrono::steady_clock::time_point time)
{
    m_sendTime = time;
}

void TCPClientRequest::setTimeout(int timeoutMs)
{
    //Проверяем корректность таймаута
    if (timeoutMs > 0)
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
    if (m_state != WaitingResponse)
    {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sendTime);

    return elapsed.count() >= m_timeoutMs;
}

std::vector<uint8_t> TCPClientRequest::serialize() const
{
    std::vector<uint8_t> result;

    //[request_id (4 байта)] [data_size (4 байта)] [data]

    //Добавляем ID запроса
    uint32_t requestIdNet = htonl(m_requestId);
    const uint8_t* requestIdBytes = reinterpret_cast<const uint8_t*>(&requestIdNet);
    result.insert(result.end(), requestIdBytes, requestIdBytes + sizeof(requestIdNet));

    //Добавляем размер данных
    uint32_t dataSizeNet = htonl(static_cast<uint32_t>(m_data.size()));
    const uint8_t* dataSizeBytes = reinterpret_cast<const uint8_t*>(&dataSizeNet);
    result.insert(result.end(), dataSizeBytes, dataSizeBytes + sizeof(dataSizeNet));

    //Добавляем данные
    if(!m_data.empty())
    {
        result.insert(result.end(), m_data.begin(), m_data.end());
    }

    return result;
}

bool TCPClientRequest::deserialize(const std::vector<uint8_t>& data)
{
    //Проверяем минимальный размер (4 байта ID + 4 байта размера)
    if(data.size() < 8)
    {
        return false;
    }

    size_t offset = 0;

    //Читаем ID запроса
    uint32_t requestIdNet;
    std::memcpy(&requestIdNet, data.data() + offset, sizeof(requestIdNet));
    m_requestId = ntohl(requestIdNet);
    offset += sizeof(requestIdNet);

    //Читаем размер данных
    uint32_t dataSizeNet;
    std::memcpy(&dataSizeNet, data.data() + offset, sizeof(dataSizeNet));
    uint32_t dataSize = ntohl(dataSizeNet);
    offset += sizeof(dataSizeNet);

    //Проверяем, что данных достаточно для чтения указанного размера
    if(data.size() < offset + dataSize)
    {
        return false;
    }

    //Читаем данные
    if(dataSize > 0)
    {
        m_data.resize(dataSize);
        std::memcpy(m_data.data(), data.data() + offset, dataSize);
    }
    else
    {
        m_data.clear();
    }

    return true;
}
