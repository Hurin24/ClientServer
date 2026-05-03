#ifndef TCP_SERVER_RESPONCE_H
#define TCP_SERVER_RESPONCE_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <memory>

class TCPServerResponce
{

public:
    TCPServerResponce();
    ~TCPServerResponce();

    TCPServerResponce(const TCPServerResponce&) = delete;
    TCPServerResponce& operator=(const TCPServerResponce&) = delete;

    TCPServerResponce(TCPServerResponce&& other);
    TCPServerResponce& operator=(TCPServerResponce&& other);


    //Эта функция сериализует данные и возвращает их в виде вектора байт, готового для отправки по сети
    std::vector<uint8_t> serialize() const;

    //Эта функция десериализует данные и возвращает указатель на новый объект TCPServerResponce
    std::shared_ptr<TCPServerResponce> deserialize(const std::vector<uint8_t>& data);


private:
    uint64_t m_requestID = 0;       //Уникальный идентификатор ответа
    std::vector<uint8_t> m_data;    //Данные ответа
};

#endif //TCP_SERVER_RESPONCE_H
