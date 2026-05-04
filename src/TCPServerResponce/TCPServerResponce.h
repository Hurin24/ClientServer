#ifndef TCP_CLIENT_REQUEST_H
#define TCP_CLIENT_REQUEST_H

#include "TCPClientServerProtocol.h"

#include <vector>
#include <cstdint>
#include <memory>

class TCPClientRequest;

class TCPServerResponce
{

public:
    TCPServerResponce();
    ~TCPServerResponce();

    TCPServerResponce(TCPServerResponce& other) = delete;
    TCPServerResponce& operator=(TCPServerResponce& other) = delete;

    TCPServerResponce(TCPClientRequest&& other);
    TCPServerResponce(TCPServerResponce&& other);
    TCPServerResponce& operator=(TCPClientRequest&& other);
    TCPServerResponce& operator=(TCPServerResponce&& other);

    uint32_t getRequestID() const;
    TCPClientServerProtocol::ResponceStatus getResponceStatus() const;
    const std::vector<uint8_t>& getData() const;

    void setData(const std::vector<uint8_t>& data);
    void setData(std::vector<uint8_t>&& data);

    //Сериализация
    std::vector<uint8_t> serialize() const;
    static std::shared_ptr<TCPServerResponce> deserialize(std::vector<uint8_t>& data);

private:
    size_t m_responceID = 0;                                        //Уникальный идентификатор ответа
    TCPClientServerProtocol::ResponceStatus m_responceStatus = TCPClientServerProtocol::ResponceStatus::Success;    //Результат ответа сервера
    std::vector<uint8_t> m_data;                                    //Данные ответа
};

#endif //TCP_CLIENT_REQUEST_H
