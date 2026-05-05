#include "TCPServerResponse.h"
#include "../TCPClientRequest/TCPClientRequest.h"

#include <cstring>
#include <arpa/inet.h>

TCPServerResponse::TCPServerResponse() :
                   m_responseID(0),
                   m_responseStatus(TCPClientServerProtocol::ResponseStatus::Success)
{

}

TCPServerResponse::~TCPServerResponse()
{

}

TCPServerResponse::TCPServerResponse(TCPClientRequest&& other) :
                   m_responseID(other.getRequestID()),
                   m_responseStatus(TCPClientServerProtocol::ResponseStatus::Success),
                   m_data(std::move(other.getData()))

{

}

TCPServerResponse::TCPServerResponse(TCPServerResponse&& other) :
                   m_responseID(other.m_responseID),
                   m_responseStatus(TCPClientServerProtocol::ResponseStatus::Success),
                   m_data(std::move(other.m_data))
{
    other.m_responseID = 0;
}

TCPServerResponse& TCPServerResponse::operator=(TCPServerResponse&& other)
{
    if(this != &other)
    {
        m_responseID = other.m_responseID;
        m_responseStatus = TCPClientServerProtocol::ResponseStatus::Success;
        m_data = std::move(other.m_data);

        other.m_responseID = 0;
    }

    return *this;
}

uint32_t TCPServerResponse::getResponseID() const
{
    return m_responseID;
}

TCPClientServerProtocol::ResponseStatus TCPServerResponse::getResponseStatus() const
{
    return m_responseStatus;
}

const std::vector<uint8_t>& TCPServerResponse::getData() const
{
    return m_data;
}

TCPServerResponse::TCPServerResponseState TCPServerResponse::getState() const
{
    return m_state;
}

void TCPServerResponse::setData(const std::vector<uint8_t>& data)
{
    m_data = data;
}

void TCPServerResponse::setData(std::vector<uint8_t>&& data)
{
    m_data = std::move(data);
}

void TCPServerResponse::setState(TCPServerResponseState newState)
{
    m_state = newState;
}

std::vector<uint8_t> TCPServerResponse::serialize() const
{
    using namespace TCPClientServerProtocol;

    std::vector<uint8_t> result;

    //Создаем заголовок ответа
    ResponseHeader header;
    header.dataSize = m_data.size() + sizeof(ResponseHeader);
    header.id = m_responseID;
    header.status = m_responseStatus;


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

std::shared_ptr<TCPServerResponse> TCPServerResponse::deserialize(std::vector<uint8_t>& data)
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
    auto response = std::shared_ptr<TCPServerResponse>(new TCPServerResponse);
    response->m_responseID = id;

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