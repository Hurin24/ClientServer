#include "Session.h"

Session::Session()
{

}

Session::~Session()
{

}

Session::Session(Session&& other)
{

}

Session& Session::operator=(Session&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    return *this;
}