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

    //Эта функция обработчик
    virtual void onResponseRecieved(/*Тут должны быть параметры пока не придумал*/) = 0;
};

#endif //SESSION_H
