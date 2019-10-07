#include "MessageBuilder.h"

MessageBuilder::MessageBuilder(MessageProcessor *m) : msgProcessor(m), log(nullptr)
{
    lastDataTime=static_cast<unsigned long>(time(NULL));
    msgLength=0;
    p=0;
    checkSum=0;
    countToFour=0;
    targetCheckSum=0;
    log=nullptr;
}

void MessageBuilder::setLogger(Logger *l)
{
    log=l;
}

MessageBuilder::~MessageBuilder()
{
    if (p>0) delete[] message;
}

bool MessageBuilder::addByte(byte b)
{
    lastDataTime=static_cast<unsigned long>(time(NULL));
    if (p==0 && countToFour<4) // we have a new message
    {
        msgLength=msgLength*256+b;
        if (msgLength>maxMessageLength) goto error;
        countToFour++;
        return true;
    }
    else if (msgLength>0 && p==0 && countToFour==4) // start new request
    {
        if (msgLength>maxMessageLength) goto error;
        message = new byte[msgLength];
        message[p]=b;
        p++;
        countToFour = 0;
        checkSum = addModulo(checkSum, b);
        return true;
    }
    else if (p>0 && p<msgLength && countToFour==0) // continue to build message
    {
        message[p]=b;
        p++;
        checkSum = addModulo(checkSum, b);
        return true;
    }
    else if (p>0 && p==msgLength) // end of message
    {
        if (countToFour<4)
        {
            targetCheckSum=targetCheckSum*256+b;
            countToFour++;
        }
        if (countToFour<4) return true;
        else // checking the checksum
        {
            const bool success = (targetCheckSum==checkSum);
            if (!success) logError("MessageBuilder::addByte: checksum not ok");
            if (success && msgProcessor!=nullptr) msgProcessor->process(msgLength, message);
            delete[] message;
            msgLength=0;
            countToFour=0;
            targetCheckSum=0;
            checkSum=0;
            p=0;
            return success;
        }
    }
error:
    msgLength=0;
    countToFour=0;
    targetCheckSum=0;
    checkSum=0;
    if (p>0) delete[] message;
    p=0;
    logError("Error in MessageBuilder::addByte(byte b)");
    return false;
}

unsigned long MessageBuilder::addModulo(unsigned long a, unsigned long b)
{
    const unsigned long diff = maxLong - a;
    if (b>diff) return b-diff-1;
    else return a+b;
}

unsigned long MessageBuilder::getLastDataTime()
{
    return lastDataTime;
}

void MessageBuilder::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void MessageBuilder::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}
