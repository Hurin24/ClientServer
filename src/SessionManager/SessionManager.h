#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <thread>
#include <mutex>
#include <list>


class Session;

class SessionManager
{

public:
    SessionManager();
    ~SessionManager();

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    SessionManager(SessionManager&& other);
    SessionManager& operator=(SessionManager&& other);


protected:
    bool m_isWorking = true;   //Флаг, указывающий на то, что SessionManager работает

    virtual bool execute();    //Рабочий цикл SessionManager


private:
    std::mutex m_sessionsListMutex;
    std::list<Session> m_sessionsList;

    std::thread m_thread;                   //Поток для обработки сессии
};

#endif //SESSION_MANAGER_H
