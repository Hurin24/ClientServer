#ifndef TCP_CLIENT_REQUEST_H
#define TCP_CLIENT_REQUEST_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <memory>

class TCPClientRequest
{

public:
    TCPClientRequest();
    ~TCPClientRequest();

    TCPClientRequest(TCPClientRequest&& other);
    TCPClientRequest& operator=(TCPClientRequest&& other);

    //Эта функция создаёт новый ID для запроса
    static size_t generateNewRequestID();

    enum TCPClientRequestState
    {
        Pending,            //Ожидает отправки (в очереди)
        Sending,            //Отправляется в данный момент
        WaitingResponse,    //Отправлен, ожидается ответ
        ResponseReceived,   //Получен ответ, запрос успешно завершен
        Failed,             //Ошибка при выполнении запроса
        TimedOut            //Таймаут ожидания ответа
    };


    uint32_t getRequestID() const;
    const std::vector<uint8_t>& getData() const;

    TCPClientRequestState getState() const;
    std::chrono::steady_clock::time_point getSendTime() const;
    int getTimeout() const;

    void setRequestID(size_t ID);
    void setData(const std::vector<uint8_t>& data);
    void setData(std::vector<uint8_t>&& data);

    void setState(TCPClientRequestState state);
    void setSendTime(std::chrono::steady_clock::time_point time);
    void setTimeout(int timeoutMs);

    bool isTimeoutExpired() const;  //Проверяет, истек ли таймаут

    //Сериализация
    std::vector<uint8_t> serialize() const;
    static std::shared_ptr<TCPClientRequest> deserialize(std::vector<uint8_t>& data);


private:
    static std::atomic<size_t> m_nextRequestID;                     //Атомарный счетчик для генерации ID

    size_t m_requestID = 0;                                         //Уникальный идентификатор запроса
    std::vector<uint8_t> m_data;                                    //Данные запроса

    TCPClientRequestState m_state = Pending;                        //Состояние запроса

    std::chrono::steady_clock::time_point m_sendTime;               //Время отправки запроса

    int m_timeoutMs = 5000;                                         //Таймаут ожидания ответа (мс)
};

#endif //TCP_CLIENT_REQUEST_H
