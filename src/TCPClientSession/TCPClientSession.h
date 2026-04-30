#ifndef TCP_CLIENT_SESSION_H
#define TCP_CLIENT_SESSION_H

#include "../ClientSession/ClientSession.h"
#include "../TCPSocket/TCPSocket.h"

#include <condition_variable>
#include <list>


class TCPClientRequest;
class TCPServerResponse;

class TCPClientSession : public ClientSession
{

public:
    TCPClientSession();
    ~TCPClientSession();

    TCPClientSession(const TCPClientSession&) = delete;
    TCPClientSession& operator=(const TCPClientSession&) = delete;

    TCPClientSession(TCPClientSession&& other);
    TCPClientSession& operator=(TCPClientSession&& other);


    //Эта функция выполняет подключение к серверу
    virtual bool connect(std::string serverIp, int serverPort) override;

    //Эта функция выполняет отключения от сервера
    virtual bool disconnect() override;

    //Эта функция выполняет запрос к серверу
    virtual bool sendRequest(ClientRequest&& clientRequest) override;


protected:
    //Эта функция возвращает флаг говорящий о том есть ли запросы, которые нужно отправить
    virtual bool isHasRequest() override;

    //Эта функция принимает и парсит ответ от сервера
    virtual void onResponseRecieved(const ServerResponse& serverResponse) override;


private:
    TCPSocket m_socket;                     //Сокет, связанный с сессией
    std::string m_serverIp;                 //IP-адрес сервера
    int m_serverPort;                       //Порт сервера

    std::mutex m_queueMutex;                //Мьютекс для защиты очереди запросов на отправку
    std::list<TCPClientRequest> m_queue;    //Очередь запросов на отправку
};

#endif //TCP_CLIENT_SESSION_H
