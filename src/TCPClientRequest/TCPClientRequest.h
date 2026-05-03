#ifndef TCP_CLIENT_REQUEST_H
#define TCP_CLIENT_REQUEST_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <atomic>

class TCPClientRequest
{

public:
    TCPClientRequest();
    ~TCPClientRequest();

    TCPClientRequest(const TCPClientRequest&) = delete;
    TCPClientRequest& operator=(const TCPClientRequest&) = delete;

    TCPClientRequest(TCPClientRequest&& other);
    TCPClientRequest& operator=(TCPClientRequest&& other);

    enum TCPClientRequestState
    {
        Pending,            //Ожидает отправки (в очереди)
        Sending,            //Отправляется в данный момент
        WaitingResponse,    //Отправлен, ожидается ответ
        ResponseReceived,   //Получен ответ, запрос успешно завершен
        Failed,             //Ошибка при выполнении запроса
        TimedOut            //Таймаут ожидания ответа
    };


    TCPClientRequestState getState() const;
    const std::vector<uint8_t>& getData() const;
    uint32_t getRequestID() const;
    std::chrono::steady_clock::time_point getSendTime() const;
    int getTimeout() const;

    // Сеттеры
    void setState(TCPClientRequestState state);
    void setData(const std::vector<uint8_t>& data);
    void setData(std::vector<uint8_t>&& data);
    void setRequestID(uint32_t ID);
    void setSendTime(std::chrono::steady_clock::time_point time);
    void setTimeout(int timeoutMs);

    bool isTimeoutExpired() const;  // Проверяет, истек ли таймаут

    //Сериализация
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);


private:
    static std::atomic<uint64_t> m_nextID;                          //Атомарный счетчик для генерации ID

    std::vector<uint8_t> m_data;                                    //Данные запроса
    TCPClientRequestState m_state = Pending;                        //Состояние запроса
    uint32_t m_requestId = 0;                                       //Уникальный идентификатор запроса
    std::chrono::steady_clock::time_point m_sendTime;               //Время отправки запроса

    int m_timeoutMs = 5000;                                         //Таймаут ожидания ответа (мс)
};

#endif //TCP_CLIENT_REQUEST_H
