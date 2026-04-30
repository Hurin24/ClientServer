#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <thread>

class Application;

class Session
{

public:
    Session();
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Session(Session&& other);
    Session& operator=(Session&& other);


};

#endif //SESSION_H
