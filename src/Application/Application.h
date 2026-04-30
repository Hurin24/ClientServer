#ifndef APPLICATION_H
#define APPLICATION_H

class Application
{

public:
    Application();
    ~Application();

    //Запускает выполнение приложения
    virtual int execute() = 0;
};

#endif //APPLICATION_H
