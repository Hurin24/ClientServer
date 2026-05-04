#ifndef TCP_SERVER_RESPONSE_H
#define TCP_SERVER_RESPONSE_H

#include "TCPClientServerProtocol.h"

#include <vector>
#include <cstdint>
#include <memory>

class TCPClientRequest;

class TCPServerResponse
{

public:
    TCPServerResponse();
    ~TCPServerResponse();

    TCPServerResponse(TCPServerResponse& other) = delete;
    TCPServerResponse& operator=(TCPServerResponse& other) = delete;

    TCPServerResponse(TCPClientRequest&& other);
    TCPServerResponse(TCPServerResponse&& other);
    TCPServerResponse& operator=(TCPServerResponse&& other);


    enum TCPServerResponseState
    {
        Pending,            //Ожидает отправки (в очереди)
        Sending,            //Отправляется в данный момент
        WasSended,          //Был отправлен
        Failed              //Ошибка при отправке
    };


    uint32_t getResponseID() const;
    TCPClientServerProtocol::ResponseStatus getResponseStatus() const;
    const std::vector<uint8_t>& getData() const;

    TCPServerResponseState getState() const;


    void setData(const std::vector<uint8_t>& data);
    void setData(std::vector<uint8_t>&& data);

    void setState(TCPServerResponseState newState);


    //Сериализация
    std::vector<uint8_t> serialize() const;
    static std::shared_ptr<TCPServerResponse> deserialize(std::vector<uint8_t>& data);


private:
    size_t m_responseID = 0;                                        //Уникальный идентификатор ответа
    TCPClientServerProtocol::ResponseStatus m_responseStatus = TCPClientServerProtocol::ResponseStatus::Success;    //Результат ответа сервера
    std::vector<uint8_t> m_data;                                    //Данные ответа

    TCPServerResponseState m_state = Pending;                       //Состояние ответа
};

#endif //TCP_SERVER_RESPONSE_H
