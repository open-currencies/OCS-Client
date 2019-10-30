#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include "Logger.h"
#include "Type14Entry.h"
#include "Type13Entry.h"
#include "Type12Entry.h"
#include "CIDsSet.h"
#include <list>
#include <string>
#include "secblock.h"
#include "pssr.h"
#include "sha3.h"
#include "Util.h"

typedef unsigned char byte;
using namespace std;

class ConnectionHandling;

class MessageProcessor
{
public:
    MessageProcessor();
    ~MessageProcessor();
    void process(const int n, byte *message);
    void setLogger(Logger *l);
    void setConnectingHandling(ConnectionHandling *c);
    unsigned long addRqstAnticipation(string &rqst);
    list<pair<Type12Entry*, CIDsSet*>>* getProcessedRqst(unsigned long rqstNum);
    string* getProcessedRqstStr(unsigned long rqstNum);
    void deleteOldRqst(unsigned long rqstNum);
    void ignoreOldRqst(unsigned long rqstNum);
    static void deleteList(list<pair<Type12Entry*, CIDsSet*>>* elem);
protected:
private:
    Util u;
    ConnectionHandling *ch;
    void considerContactInfoRequest(const size_t n, byte *message);
    void heartBeatMsg(const size_t n, byte *message);
    void pblcKeyInfoMsg(const size_t n, byte *message);
    void currOrOblInfoMsg(const size_t n, byte *message);
    void idInfoMsg(const size_t n, byte *message);
    void claimsMsg(const size_t n, byte *message);
    void nextClaimMsg(const size_t n, byte *message);
    void transactionsMsg(const size_t n, byte *message);
    void decThreadsMsg(const size_t n, byte *message);
    void refInfoMsg(const size_t n, byte *message);
    void essentialsMsg(const size_t n, byte *message);
    void notaryInfoMsg(const size_t n, byte *message);
    void isBannedMsg(const size_t n, byte *message);
    Logger *log;
    void logInfo(const char *msg);
    void logError(const char *msg);
    static void deleteList(list<Type13Entry*> type13entries);
    Type12Entry* createT12FromT13Str(string& str);
    Type12Entry* extractType12Entry(string &t13eList, CIDsSet &firstIDs, const bool keysAlreadyLocked, const bool firstBlockOnly);
    list<pair<Type12Entry*, CIDsSet*>>* extractType12Entries(string &t13eList, const bool keysAlreadyLocked);
    // members for storing received data
    mutex rqststore_mutex;
    unsigned long nextRqstNum;
    map<string, unsigned long> anticipatedRqstByStr;
    map<unsigned long, string> anticipatedRqstByNum;
    map<unsigned long, list<pair<Type12Entry*, CIDsSet*>>*> processedRqsts;
    bool storeProcessedRqst(string &rqst, list<pair<Type12Entry*, CIDsSet*>>* result);
    map<unsigned long, string*> processedRqstsStr;
    bool storeProcessedRqstStr(string &rqst, string* result);
};

#endif // MESSAGEPROCESSOR_H
