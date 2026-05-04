#include "TCPServerResponce.h"
#include "../TCPClientRequest/TCPClientRequest.h"

#include <cstring>
#include <arpa/inet.h>

TCPServerResponce::TCPServerResponce() :
                   m_responceID(0),
                   m_responceStatus(TCPClientServerProtocol::ResponceStatus::Success)
{

}

TCPServerResponce::~TCPServerResponce()
{

}

TCPServerResponce::TCPServerResponce(TCPClientRequest&& other) :
                   m_responceID(other.getRequestID()),
                   m_responceStatus(TCPClientServerProtocol::ResponceStatus::Success),
                   m_data(std::move(other.getData()))

{

}

TCPServerResponce::TCPServerResponce(TCPServerResponce&& other) :
                   m_responceID(other.m_responceID),
                   m_responceStatus(TCPClientServerProtocol::ResponceStatus::Success),
                   m_data(std::move(other.m_data))
{
    other.m_responceID = 0;
}

TCPServerResponce& TCPServerResponce::operator=(TCPClientRequest&& other)
{
    if(this != &other)
    {
        m_responceID = other.getRequestID();
        m_responceStatus = TCPClientServerProtocol::ResponceStatus::Success;
        m_data = std::move(other.getData());
    }

    return *this;
}

TCPServerResponce& TCPServerResponce::operator=(TCPServerResponce&& other)
{
    if(this != &other)
    {
        m_responceID = other.m_responceID;
        m_responceStatus = TCPClientServerProtocol::ResponceStatus::Success;
        m_data = std::move(other.m_data);

        other.m_responceID = 0;
    }

    return *this;
}

uint32_t TCPServerResponce::getRequestID() const
{
    return m_responceID;
}

TCPClientServerProtocol::ResponceStatus TCPServerResponce::getResponceStatus() const
{
    return m_responceStatus;
}

const std::vector<uint8_t>& TCPServerResponce::getData() const
{
    return m_data;
}

void TCPServerResponce::setData(const std::vector<uint8_t>& data)
{
    m_data = data;
}

void TCPServerResponce::setData(std::vector<uint8_t>&& data)
{
    m_data = std::move(data);
}

std::vector<uint8_t> TCPServerResponce::serialize() const
{
    using namespace TCPClientServerProtocol;

    std::vector<uint8_t> result;

    //Создаем заголовок ответа
    ResponseHeader header;
    header.dataSize = m_data.size() + sizeof(ResponseHeader);
    header.id = m_responceID;
    header.status = m_responceStatus;


    //Сериализуем заголовок в сетевой порядок байт
    ResponseHeader headerNet;
    headerNet.id = htobe64(header.id);
    headerNet.status = header.status;
    headerNet.dataSize = htobe64(header.dataSize);

    const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&headerNet);
    result.insert(result.end(), headerBytes, headerBytes + sizeof(ResponseHeader));

    //Добавляем данные
    if(!m_data.empty())
    {
        result.insert(result.end(), m_data.begin(), m_data.end());
    }

    return result;
}

std::shared_ptr<TCPServerResponce> TCPServerResponce::deserialize(std::vector<uint8_t>& data)
{
    using namespace TCPClientServerProtocol;

    //Если данных для заголовка недостаточно
    if(data.size() < sizeof(ResponseHeader))
    {
        return nullptr;
    }

    //Кастуем начало данных к ResponseHeader
    ResponseHeader* header = reinterpret_cast<ResponseHeader*>(data.data());

    //Преобразуем из сетевого порядка байт в host порядок
    uint64_t id = be64toh(header->id);
    uint64_t dataSize = be64toh(header->dataSize);

    //Вычисляем полный размер пакета
    size_t totalSize = sizeof(ResponseHeader) + dataSize;

    //Если данных для полного пакета недостаточно
    if(data.size() < totalSize)
    {
        return nullptr;
    }

    //Создаем объект ответа
    auto response = std::shared_ptr<TCPServerResponce>(new TCPServerResponce);
    response->m_responceID = id;

    //Копируем данные со смещением(без заголовка)
    if(dataSize > 0)
    {
        response->m_data.resize(dataSize);
        std::copy(data.begin() + sizeof(ResponseHeader),
                  data.begin() + totalSize,
                  response->m_data.begin());
    }


    return response;
}