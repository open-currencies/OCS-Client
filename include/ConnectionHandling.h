#ifndef CONNECTIONHANDLING_H
#define CONNECTIONHANDLING_H

//#include <winsock2.h> // under mingw
#include <unistd.h>
#include "ContactInfo.h"
#include "MessageBuilder.h"
#include "RequestBuilder.h"
#include <pthread.h>
#include <mutex>
#include <map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> // under linux
#include <netinet/in.h> // under linux
#include <arpa/inet.h> // under linux
#include "nlohmann/json.hpp"
#include <sys/stat.h>
#include <fstream>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include "Logger.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <sstream>

using namespace std;
using json = nlohmann::json;

class MessageProcessor;

class ConnectionHandling
{
public:
    ConnectionHandling(RequestBuilder *r, Fl_Box *s);
    void startConnector(MessageProcessor* m);
    ~ConnectionHandling();
    void stopSafely();
    void setLogger(Logger *l);
    RequestBuilder* getRqstBuilder();
    MessageProcessor* getMsgProcessor();
    unsigned long getNotaryNr();
    bool sendRequest(string &rqst);
    bool connectionEstablished();
    bool tryToStoreContactInfo(ContactInfo &contactInfo, bool checkSignature);
    void banCurrentNotary();
protected:
private:
    RequestBuilder *rqstBuilder;
    mutex connection_mutex;
    Fl_Box *statusbar;
    volatile MessageProcessor* msgProcessor;
    Logger *log;
    pthread_t connectorThread;
    string displayedLabel;

    // general connections data
    void loadDataFromConnectionsFile();
    string nonNotarySourceUrl;
    map<unsigned long, ContactInfo*> servers;
    void saveToFile();
    volatile bool reachOutRunning;
    volatile bool reachOutStopped;
    map<unsigned long, unsigned long long> notariesBans;

    // requests buffer data
    mutex requests_buffer_mutex;
    string requestsStr;
    void addRequest(string &rqst);
    size_t requestsStrLength();

    // data for the established connection
    volatile unsigned long connectedNotary;
    volatile int socketNr;
    volatile bool listenOnSocket;
    volatile unsigned long long lastConnectionTime;
    volatile unsigned long long lastListeningTime;
    pthread_t listenerThread;
    MessageBuilder* msgBuilder;
    volatile bool socketReaderStopped;

    // routines
    unsigned long getSomeEligibleNotary();
    struct HandlerNotaryPair
    {
        ConnectionHandling* cHandling;
        unsigned long notary;
        HandlerNotaryPair(ConnectionHandling* c, unsigned long n);
    };
    static void *connectToNotaryRoutine(void *handlerNotaryPair);
    void condSleep(unsigned long targetSleepTimeInSec);
    void displayStatus(const char * statStr, bool connected);
    static void *reachOutRoutine(void *connectionHandling);
    static void *socketReader(void *connectionHandling);
    static void closeconnection(int sock);

    // for time out thread
    pthread_t attemptInterruptionThread;
    volatile bool attemptInterrupterStopped;
    static void *attemptInterrupter(void *connectionHandling);
    volatile int socketNrInAttempt;

    // for downloads
    pthread_t downloadThread;
    static void *downloadRoutine(void *connectionHandling);
    volatile bool downloadRunning;
    volatile bool downloadStopped;
    static bool downloadFile(string &url, string &output);
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
    static unsigned long long systemTimeInMs();
    void logInfo(const char *msg);
    void logError(const char *msg);
};

#endif // CONNECTIONHANDLING_H
