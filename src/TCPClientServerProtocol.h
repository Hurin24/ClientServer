#ifndef TCP_CLIENT_SERVER_PROTOCOL_H
#define TCP_CLIENT_SERVER_PROTOCOL_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

namespace TCPClientServerProtocol
{
    //Константы протокола
    constexpr ssize_t DATA_SIZE = sizeof(ssize_t);                              //Размер данных(ssize_t)
    constexpr ssize_t ID_SIZE = sizeof(ssize_t);                                //ID(ssize_t)
    constexpr ssize_t STATUS_SIZE = sizeof(uint8_t);                            //1 байт на статус
    constexpr ssize_t REQUEST_HEADER_SIZE = DATA_SIZE + ID_SIZE;                //sizeof(ssize_t) + sizeof(ssize_t) на заголовок запроса
    constexpr ssize_t RESPONSE_HEADER_SIZE = DATA_SIZE + ID_SIZE + STATUS_SIZE; //sizeof(ssize_t) + sizeof(ssize_t) + sizeof(uint8_t) на заголовок ответа


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
        ssize_t dataSize;        //Размер данных
        ssize_t id;              //Уникальный идентификатор запроса
    };
    #pragma pack(pop)

    //Структура заголовка ответа
    #pragma pack(push, 1)
    struct ResponseHeader
    {
        ssize_t dataSize;        //Размер данных
        ssize_t id;              //Уникальный идентификатор ответа, совпадает с ID запроса
        ResponseStatus status;  //Статус
    };
    #pragma pack(pop)


    //Функция для определения, сколько байт нужно дочитать
    inline ssize_t howMuchNeed(const uint8_t* const ptrData, ssize_t dataSize)
    {
        //Если данных меньше DATA_SIZE
        if(dataSize < DATA_SIZE)
        {
            //Данных меньше чем DATA_SIZE

            //Возвращаем, сколько байт нужно дочитать для получения полного запроса
            return DATA_SIZE - dataSize;
        }

        //Данных больше или равно DATA_SIZE

        //Читаем размер данных
        ssize_t tempDataSize;
        std::memcpy(&tempDataSize, ptrData, DATA_SIZE);

        //Возвращаем, сколько байт нужно дочитать для получения полного запроса
        return tempDataSize - dataSize ;
    }
}

#endif //TCP_CLIENT_SERVER_PROTOCOL_H/
