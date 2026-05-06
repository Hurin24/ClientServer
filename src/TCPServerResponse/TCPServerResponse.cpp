#include "TCPServerResponse.h"
#include "../TCPClientRequest/TCPClientRequest.h"

#include <cstring>
#include <arpa/inet.h>

TCPServerResponse::TCPServerResponse() :
                   m_responseID(0),
                   m_responseStatus(TCPClientServerProtocol::ResponseStatus::Success),
                   m_state(Pending)
{

}

TCPServerResponse::~TCPServerResponse()
{

}

TCPServerResponse::TCPServerResponse(TCPClientRequest&& other) :
                   m_responseID(other.getRequestID()),
                   m_responseStatus(TCPClientServerProtocol::ResponseStatus::Success),
                   m_data(std::move(other.getData())),
                   m_state(Pending)

{

}

TCPServerResponse::TCPServerResponse(TCPServerResponse&& other) :
                   m_responseID(other.m_responseID),
                   m_responseStatus(other.m_responseStatus),
                   m_data(std::move(other.m_data)),
                   m_state(other.m_state)
{
    //Обнуляем исходный объект
    other.m_responseID = 0;
    other.m_state = TCPServerResponse::Failed;
}

TCPServerResponse& TCPServerResponse::operator=(TCPServerResponse&& other)
{
    //Проверяем на самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    //Перемещаем данные из other в текущий объект
    m_responseID = other.m_responseID;
    m_responseStatus = other.m_responseStatus;
    m_data = std::move(other.m_data);
    m_state = other.m_state;

    //Обнуляем исходный объект
    other.m_responseID = 0;
    other.m_state = TCPServerResponse::Failed;

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

    //Добавляем поле данных
    ssize_t dataSizeNet = RESPONSE_HEADER_SIZE + m_data.size();
    const uint8_t* dataSizeBytes = reinterpret_cast<const uint8_t*>(&dataSizeNet);
    result.insert(result.end(), dataSizeBytes, dataSizeBytes + sizeof(dataSizeNet));

    //Добавляем ID ответа
    const uint8_t* responseIdBytes = reinterpret_cast<const uint8_t*>(&m_responseID);
    result.insert(result.end(), responseIdBytes, responseIdBytes + sizeof(m_responseID));

    //Добавляем результат ответа сервера
    const uint8_t* responseStatusBytes = reinterpret_cast<const uint8_t*>(&m_responseStatus);
    result.insert(result.end(), responseStatusBytes, responseStatusBytes + sizeof(m_responseStatus));

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

    //Если данных для полного пакета недостаточно
    if(data.size() < header->dataSize)
    {
        return nullptr;
    }

    //Создаем объект ответа
    auto response = std::shared_ptr<TCPServerResponse>(new TCPServerResponse);
    response->m_responseID = header->id;
    response->m_responseStatus = header->status;

    //Копируем данные со смещением(без заголовка)
    if(header->dataSize > 0)
    {
        int tempSize = header->dataSize - sizeof(ResponseHeader);

        response->m_data.resize(tempSize);
        std::copy(data.begin() + sizeof(ResponseHeader), data.begin() + tempSize, response->m_data.begin());
    }

    return response;
}
