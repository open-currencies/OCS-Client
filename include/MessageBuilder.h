#ifndef MESSAGEBUILDER_H
#define MESSAGEBUILDER_H

#include "MessageProcessor.h"
#include "Logger.h"
#include <stdint.h>
#include <time.h>

#define maxLong 4294967295
#define maxMessageLength 524288
typedef unsigned char byte;

class MessageBuilder
{
public:
    MessageBuilder(MessageProcessor *m);
    ~MessageBuilder();
    bool addByte(byte b);
    unsigned long getLastDataTime();
    void setLogger(Logger *l);
protected:
private:
    MessageProcessor *msgProcessor;
    Logger *log;
    volatile unsigned long lastDataTime;
    byte *message;
    unsigned long msgLength;
    unsigned long p;
    unsigned long checkSum;
    unsigned long targetCheckSum;
    int countToFour;

    static unsigned long addModulo(unsigned long a, unsigned long b);

    void logInfo(const char *msg);
    void logError(const char *msg);
};

#endif // MESSAGEBUILDER_H
