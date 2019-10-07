#ifndef INTERNALTHREAD_H
#define INTERNALTHREAD_H

#include "ConnectionHandling.h"
#include <climits>
#include <set>
#include <mutex>
#include <pthread.h>
#include "Logger.h"

class InternalThread
{
public:
    InternalThread(ConnectionHandling *c);
    ~InternalThread();
    void start();
    void stopSafely();
    void setLogger(Logger *l);
    void addAccountPairOfInterest(string key, string liqui);
    void addIdOfInterest(CompleteID id);
    ConnectionHandling* getConnectionHandling();
    volatile bool running;
protected:
private:
    volatile bool stopped;
    ConnectionHandling *connection;
    Logger *log;
    pthread_t thread;
    static void *routine(void *internalThread);
    void logInfo(const char *msg);
    void logError(const char *msg);
    void condSleep(unsigned long targetSleepTimeInSec);

    volatile unsigned int collectInfoCount;
    volatile unsigned int claimsInfoCount;
    volatile unsigned int idsInfoCount;
    volatile unsigned int contactsRqstCount;
    volatile unsigned int heartBeatCount;

    // of continuous interest
    mutex ofinterest_mutex;
    set<pair<string,string>> accountPairsOfInterest;
    set<CompleteID, CompleteID::CompareIDs> idsOfInterest;
};

#endif // INTERNALTHREAD_H
