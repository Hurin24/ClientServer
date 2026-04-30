#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "Session.h"

#include <string>
#include <thread>

class ServerResponse;
class ClientRequest;

class ClientSession : public Session
{

public:
    ClientSession();
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    ClientSession(ClientSession&& other);
    ClientSession& operator=(ClientSession&& other);


    //Эта функция выполняет подключение к серверу
    virtual bool connect(std::string serverIp, int serverPort) = 0;

    //Эта функция выполняет отключения от сервера
    virtual bool disconnect() = 0;

    //Эта функция выполняет запрос к серверу
    virtual bool sendRequest(ClientRequest&& clientRequest) = 0;


protected:
    //Эта функция возвращает флаг говорящий о том есть ли запросы, которые нужно отправить
    virtual bool isHasRequest() = 0;

    //Эта функция принимает и парсит ответ от сервера
    virtual void onResponseRecieved(const ServerResponse& serverResponse) = 0;
};

#endif //CLIENT_SESSION_H