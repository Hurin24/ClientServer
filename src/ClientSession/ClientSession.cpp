#include "ClientSession.h"

ClientSession::ClientSession()
{

}

ClientSession::~ClientSession()
{
    disconnect();
}

ClientSession::ClientSession(ClientSession&& other)
{

}

ClientSession& ClientSession::operator=(ClientSession&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    return *this;
}