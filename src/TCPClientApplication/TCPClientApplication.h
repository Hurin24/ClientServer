#ifndef TCP_CLIENT_APPLICATION_H
#define TCP_CLIENT_APPLICATION_H

class TCPClientApplication
{

public:
    TCPClientApplication();
    ~TCPClientApplication();

    TCPClientApplication(const TCPClientApplication&) = delete;
    TCPClientApplication& operator=(const TCPClientApplication&) = delete;

    int execute();
};

#endif //TCP_CLIENT_APPLICATION_H
