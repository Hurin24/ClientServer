#ifndef TCP_CLIENT_SERVER_PROTOCOL_H
#define TCP_CLIENT_SERVER_PROTOCOL_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

namespace TCPClientServerProtocol
{
    //Константы протокола
    constexpr size_t DATA_SIZE = sizeof(size_t);                                //Размер данных(size_t)
    constexpr size_t ID_SIZE = sizeof(size_t);                                  //ID(size_t)
    constexpr size_t STATUS_SIZE = sizeof(uint8_t);                             //1 байт на статус
    constexpr size_t REQUEST_HEADER_SIZE = DATA_SIZE + ID_SIZE;                 //sizeof(size_t) + sizeof(size_t) на заголовок запроса
    constexpr size_t RESPONSE_HEADER_SIZE = DATA_SIZE + ID_SIZE + STATUS_SIZE;  //sizeof(size_t) + sizeof(size_t) + sizeof(uint8_t) на заголовок ответа


    //Статусы ответа
    enum class ResponseStatus : uint8_t
    {
        Success = 0,    //Успешно
        Failed = 1      //Не успешно
    };

    //Структура заголовка запроса
    #pragma pack(push, 1)
    struct RequestHeader
    {
        size_t dataSize;        //Размер данных
        size_t id;              //Уникальный идентификатор запроса
    };
    #pragma pack(pop)

    //Структура заголовка ответа
    #pragma pack(push, 1)
    struct ResponseHeader
    {
        size_t dataSize;        //Размер данных
        size_t id;              //Уникальный идентификатор ответа, совпадает с ID запроса
        ResponseStatus status;  //Статус
    };
    #pragma pack(pop)


    //Функция для определения, сколько байт нужно дочитать
    size_t howMuchNeed(const std::vector<uint8_t>& data)
    {
        //Если данных меньше DATA_SIZE
        if(data.size() < DATA_SIZE)
        {
            //Данных меньше чем DATA_SIZE

            //Возвращаем, сколько байт нужно дочитать для получения полного запроса
            return DATA_SIZE - data.size();
        }


        //Данных больше или равно DATA_SIZE


        //Читаем размер данных
        size_t dataSize;
        std::memcpy(&dataSize, data.data(), DATA_SIZE);
        dataSize = be64toh(dataSize);


        //Если данных достаточно
        if(data.size() >= dataSize)
        {
            //Данных достаточно

            //Возвращаем 0, так как данных достаточно для получения полного запроса
            return 0;
        }


        //Данных не достаточно


        //Возвращаем, сколько байт нужно дочитать для получения полного запроса
        return dataSize - data.size();
    }
}

#endif //TCP_CLIENT_SERVER_PROTOCOL_H
