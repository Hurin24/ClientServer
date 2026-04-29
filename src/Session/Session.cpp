#include "Session.h"

Session::Session(Application* application) :
         m_application(application),
         m_thread(std::thread(&Session::execute, this))
{

}

Session::~Session()
{
    //Заверашем работу основного цикла сессии
    m_isWorking = false;

    //Если у потока, можно дождаться завершения
    if(m_thread.joinable())
    {
        //У потока можно дождаться завершения

        //Ожидаем завершения потока
        m_thread.join();
    }
}

Session::Session(Session&& other)
{
    //Перемещаем данные из other в this
    m_isWorking = other.m_isWorking;
    m_thread = std::move(other.m_thread);
    m_application = other.m_application;

    //Обнуляем исходный объект, чтобы он не выполнял работу по обработке сессии при своем уничтожении
    other.m_isWorking = false;
    other.m_application = nullptr;
}

Session& Session::operator=(Session&& other)
{
    //Проверяем самоприсваивание
    if(this == &other)
    {
        return *this;
    }

    //Перемещаем данные из other в this
    m_isWorking = other.m_isWorking;
    m_thread = std::move(other.m_thread);
    m_application = other.m_application;

    //Обнуляем исходный объект, чтобы он не выполнял работу по обработке сессии при своем уничтожении
    other.m_isWorking = false;
    other.m_application = nullptr;

    return *this;
}

void Session::execute()
{

}
