#include "TCPClientApplication.h"

#include <utility>

TCPClientApplication::TCPClientApplication()
{

}

TCPClientApplication::~TCPClientApplication()
{

}

TCPClientApplication::TCPClientApplication(TCPClientApplication&& other) :
                      Application(std::move(other))
{

}

TCPClientApplication& TCPClientApplication::operator=(TCPClientApplication&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    Application::operator=(std::move(other));

    return *this;
}
