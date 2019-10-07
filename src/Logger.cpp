#include "Logger.h"
#include "../easylogger/easylogger.h"

Logger::Logger()
{
    // build logger
    MAIN = new easylogger::Logger("MAIN");
    main_log.open("MAIN.log");
    MAIN->Stream(main_log);
}

Logger::~Logger()
{
    main_log.close();
    delete MAIN;
}

void Logger::error(const char *msg)
{
    LOG_ERROR(*MAIN, msg);
}

void Logger::info(const char *msg)
{
    LOG_INFO(*MAIN, msg);
}
