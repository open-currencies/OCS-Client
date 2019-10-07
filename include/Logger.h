#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

namespace easylogger
{
class Logger;
}

class Logger
{
public:
    Logger();
    ~Logger();
    void error(const char *msg);
    void info(const char *msg);
protected:
private:
    easylogger::Logger *MAIN;
    ofstream main_log;
};

#endif // LOGGER_H
