#ifndef TCP_CLIENT_APPLICATION_H
#define TCP_CLIENT_APPLICATION_H

#include "../Application/Application.h"

class TCPClientApplication : public Application
{

public:
    TCPClientApplication();
    ~TCPClientApplication();

    TCPClientApplication(const TCPClientApplication&) = delete;
    TCPClientApplication& operator=(const TCPClientApplication&) = delete;

    TCPClientApplication(TCPClientApplication&& other);
    TCPClientApplication& operator=(TCPClientApplication&& other);


protected:
    int execute() override;


private:
};

#endif //TCP_CLIENT_APPLICATION_H
