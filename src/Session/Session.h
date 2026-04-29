#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <thread>

class Application;

class Session
{

public:
    Session(Application* application);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Session(Session&& other);
    Session& operator=(Session&& other);


protected:
    bool m_isWorking = true;                //Флаг, указывающий на то, что сессия работает

    virtual bool execute();    //Рабочий цикл сессии, выполняется в отдельном потоке


private:
    std::thread m_thread;                   //Поток для обработки сессии
    Application* m_application = nullptr;   //Указатель на приложение, которому принадлежит сессия
};

#endif //SESSION_H
