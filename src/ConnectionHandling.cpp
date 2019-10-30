#include "ConnectionHandling.h"
#include "MessageProcessor.h"

#define reachOutRoutineSleepTime 1 // in seconds
#define tolerableConnectionGapInMs 20000
#define maxConnectionDurationInMs 50000
#define banTimeInMs 15000
#define connectionTimeOutInMs 5000
#define connectionTimeOutCheckInMs 50
#define checkForNewLiquis 1
#define checkForNewContacts 1

ConnectionHandling::ConnectionHandling(RequestBuilder *r, Fl_Box *s) : rqstBuilder(r), statusbar(s), log(nullptr)
{
    // predefine some other stuff
    socketNr = -1;
    listenOnSocket = false;
    socketReaderStopped = true;
    reachOutRunning = false;
    reachOutStopped = true;
    msgBuilder = nullptr;
    connectedNotary = 0;
    downloadRunning = false;
    downloadStopped = true;
    attemptInterrupterStopped = true;

    // initialize random seed
    srand((unsigned int)(systemTimeInMs() % 10000));

    // load data from file
    loadDataFromConnectionsFile();
}

// NOTE: for mingw: replace "stat" with "_stat" for struct and function

void ConnectionHandling::loadDataFromConnectionsFile()
{
    // try to load from json file
    string fileNameJ("conf/connections.json");
    struct stat fJInfo;
    if (stat(fileNameJ.c_str(), &fJInfo) != 0)
    {
        return;
    }
    ifstream fJ(fileNameJ.c_str());
    if (!fJ.good())
    {
        fJ.close();
        return;
    }
    json j;
    fJ >> j;
    fJ.close();

    // read data
    connection_mutex.lock();
    nonNotarySourceUrl = j["nonNotarySourceUrl"];
    Type1Entry *type1entry = rqstBuilder->getKeysHandling()->getType1Entry();
    if (type1entry != nullptr)
    {
        const unsigned long serversNr = j["serversNr"];
        // truncate if too many
        unsigned long limit = type1entry->getMaxActing(type1entry->latestLin());
        limit += type1entry->getMaxWaiting(type1entry->latestLin());
        unsigned long index = 1;
        if (serversNr>limit) index = serversNr-limit;
        // load servers data
        for (; index<=serversNr; index++)
        {
            string indexStr("server");
            indexStr += to_string(index);
            json server = j[indexStr];
            unsigned long notaryNr = server["notaryNr"];
            if (servers.count(notaryNr)<=0)
            {
                string ip = server["ip"];
                int port = server["port"];
                unsigned long long validSince = server["validSince"];
                string signature("na");
                TNtrNr tNtrNr(type1entry->latestLin(), notaryNr);
                ContactInfo *ci = new ContactInfo(tNtrNr, ip, port, validSince, signature);
                servers.insert(pair<unsigned long, ContactInfo*>(notaryNr, ci));
            }
        }
    }
    connection_mutex.unlock();
}

bool ConnectionHandling::tryToStoreContactInfo(ContactInfo &contactInfo, bool checkSignature)
{
    // check the notary
    TNtrNr totalNotaryNr = contactInfo.getTotalNotaryNr();
    if (!totalNotaryNr.isGood()) return false;
    unsigned long notaryNr = totalNotaryNr.getNotaryNr();
    Type1Entry *type1entry = rqstBuilder->getKeysHandling()->getType1Entry();
    if (type1entry==nullptr) return false;
    if (totalNotaryNr.getLineage() != type1entry->getLineage(contactInfo.getValidSince())) return false;
    if (totalNotaryNr.getLineage() != type1entry->latestLin()) return false;

    connection_mutex.lock();

    // compare time flags
    if (servers.count(notaryNr)>0 && servers[notaryNr]->getValidSince() >= contactInfo.getValidSince())
    {
        connection_mutex.unlock();
        return false;
    }

    // check signature
    if (checkSignature)
    {
        // get public key of notary
        CryptoPP::RSA::PublicKey *publicKey = rqstBuilder->getKeysHandling()->getNotaryPubKey(totalNotaryNr, false);
        if (publicKey == nullptr)
        {
            logError("ConnectionHandling::tryToStoreContactInfo: publicKey not found");
            connection_mutex.unlock();
            return false;
        }
        Util util;
        string signedStr;
        signedStr.append(contactInfo.getIP());
        signedStr.append(util.UlAsByteSeq(contactInfo.getPort()));
        signedStr.append(util.UllAsByteSeq(contactInfo.getValidSince()));
        string *signa = contactInfo.getSignature();
        CryptoPP::SecByteBlock signature((const byte*)signa->c_str(), signa->length());
        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA3_384>::Verifier verifier(*publicKey);

        bool result = verifier.VerifyMessage((const byte*)signedStr.c_str(),
                                             signedStr.length(), signature, signature.size());
        if (!result)
        {
            logError("ConnectionHandling::tryToStoreContactInfo: bad signature");
            connection_mutex.unlock();
            return false;
        }
    }

    // store
    string ip = contactInfo.getIP();
    string nullSignature("na");
    ContactInfo *ci = new ContactInfo(totalNotaryNr, ip, contactInfo.getPort(),
                                      contactInfo.getValidSince(), nullSignature);
    if (servers.count(notaryNr)>0)
    {
        delete servers[notaryNr];
        servers[notaryNr]=ci;
    }
    else servers.insert(pair<unsigned long, ContactInfo*>(notaryNr, ci));

    saveToFile();

    connection_mutex.unlock();

    return true;
}

// connection_mutex must be locked for this
void ConnectionHandling::saveToFile()
{
    logInfo("ConnectionHandling::saveToFile");

    json out;

    out["nonNotarySourceUrl"]=nonNotarySourceUrl;
    out["serversNr"]=servers.size();

    int c=1;
    map<unsigned long, ContactInfo*>::iterator it;
    for (it=servers.begin(); it!=servers.end(); ++it)
    {
        json server;
        server["notaryNr"] = it->first;
        server["ip"] = it->second->getIP();
        server["port"] = it->second->getPort();
        server["validSince"] = it->second->getValidSince();

        string serverName("server");
        serverName += to_string(c);
        out[serverName] = server;
        c++;
    }

    // try to save to json file
    string fileNameJ("conf/connections.json");
    ofstream fJ(fileNameJ.c_str());
    if (!fJ.good())
    {
        fJ.close();
        return;
    }
    fJ << setw(4) << out << endl;
    fJ.close();
}

RequestBuilder* ConnectionHandling::getRqstBuilder()
{
    return rqstBuilder;
}

MessageProcessor* ConnectionHandling::getMsgProcessor()
{
    return ((MessageProcessor*)msgProcessor);
}

unsigned long ConnectionHandling::getNotaryNr()
{
    if (!connectionEstablished()) return 0;
    return connectedNotary;
}

void ConnectionHandling::setLogger(Logger *l)
{
    log=l;
}

ConnectionHandling::~ConnectionHandling()
{
    if (msgBuilder!=nullptr)
    {
        delete msgBuilder;
        msgBuilder=nullptr;
    }
}

void ConnectionHandling::startConnector(MessageProcessor* m)
{
    connection_mutex.lock();
    msgProcessor = m;
    ((MessageProcessor*)msgProcessor)->setConnectingHandling(this);
    reachOutRunning=true;

    // start connectorThread
    if(pthread_create(&connectorThread, NULL, reachOutRoutine, (void*) this) < 0)
    {
        logError("could not create server connector thread");
        connection_mutex.unlock();
        exit(EXIT_FAILURE);
    }
    pthread_detach(connectorThread);
    logInfo("connectorThread in  ConnectionHandling started");

    // start download thread
    downloadRunning=true;
    if(pthread_create(&downloadThread, NULL, downloadRoutine, (void*) this) < 0)
    {
        connection_mutex.unlock();
        exit(EXIT_FAILURE);
    }
    pthread_detach(downloadThread);

    connection_mutex.unlock();
}

void ConnectionHandling::stopSafely()
{
    downloadRunning=false;
    reachOutRunning=false;
    listenOnSocket=false;
    closeconnection(socketNr);
    closeconnection(socketNrInAttempt);
    while (!reachOutStopped || !downloadStopped
            || !socketReaderStopped || !attemptInterrupterStopped) usleep(100000);
}

void ConnectionHandling::closeconnection(int sock)
{
    if (sock==-1) return;
    shutdown(sock,2);
    close(sock);
}

void ConnectionHandling::banCurrentNotary()
{
    connection_mutex.lock();

    unsigned long notaryNr = connectedNotary;
    if (notaryNr<=0)
    {
        connection_mutex.unlock();
        return;
    }
    unsigned long long banUntilTime = systemTimeInMs() + banTimeInMs;
    if (notariesBans.count(notaryNr)<=0)
    {
        notariesBans.insert(pair<unsigned long, unsigned long long>(notaryNr, banUntilTime));
    }
    else notariesBans[notaryNr] = banUntilTime;

    connection_mutex.unlock();
}

bool ConnectionHandling::sendRequest(string &rqst)
{
    if (!reachOutRunning || !connectionEstablished() || rqst.length()<=0) return false;

    connection_mutex.lock();

    if (connectedNotary<=0)
    {
        connection_mutex.unlock();
        return false;
    }
    addRequest(rqst);
    // reconnect
    HandlerNotaryPair* hnpair = new HandlerNotaryPair(this, connectedNotary);
    pthread_t connectToThread;
    if(pthread_create(&connectToThread, NULL, connectToNotaryRoutine, (void*) hnpair) >= 0)
    {
        pthread_detach(connectToThread);
    }

    connection_mutex.unlock();
    return true;
}

bool ConnectionHandling::connectionEstablished()
{
    connection_mutex.lock();

    unsigned long notaryNr = connectedNotary;
    const unsigned long long currentTime = systemTimeInMs();
    if (notaryNr<=0
            || (notariesBans.count(notaryNr)>0 && notariesBans[notaryNr]>currentTime))
    {
        if (listenOnSocket) closeconnection(socketNr);
        listenOnSocket = false;
        connectedNotary = 0;
        connection_mutex.unlock();
        return false;
    }
    else if (!listenOnSocket)
    {
        if (lastListeningTime + tolerableConnectionGapInMs < currentTime)
        {
            connectedNotary = 0;
            connection_mutex.unlock();
            return false;
        }
        else if (lastListeningTime + tolerableConnectionGapInMs/2 < currentTime || requestsStrLength()>0)
        {
            // reconnect
            HandlerNotaryPair* hnpair = new HandlerNotaryPair(this, connectedNotary);
            pthread_t connectToThread;
            if(pthread_create(&connectToThread, NULL, connectToNotaryRoutine, (void*) hnpair) >= 0)
            {
                pthread_detach(connectToThread);
            }
            connection_mutex.unlock();
            return true;
        }
    }
    else if (lastConnectionTime + maxConnectionDurationInMs < currentTime)
    {
        connectedNotary = 0;
        listenOnSocket=false;
        closeconnection(socketNr);
        connection_mutex.unlock();
        return false;
    }
    else
    {
        const unsigned long long currentSyncTime = rqstBuilder->currentSyncTimeInMs();
        Type1Entry *type1entry = rqstBuilder->getKeysHandling()->getType1Entry();
        if (currentSyncTime>0 && !type1entry->isActingNotary(notaryNr, currentSyncTime))
        {
            // ban this notary for now
            unsigned long long banUntilTime = currentTime + banTimeInMs;
            if (notariesBans.count(notaryNr)<=0)
            {
                notariesBans.insert(pair<unsigned long, unsigned long long>(notaryNr, banUntilTime));
            }
            else notariesBans[notaryNr] = banUntilTime;
            // close connection
            connectedNotary = 0;
            listenOnSocket=false;
            closeconnection(socketNr);
            connection_mutex.unlock();
            return false;
        }
    }

    connection_mutex.unlock();
    return true;
}

void ConnectionHandling::displayStatus(const char * statStr, bool connected)
{
    string txt(statStr);
    string oldLabel(displayedLabel);
    if (txt.compare(oldLabel)!=0)
    {
        displayedLabel = txt;
        if (connected)
        {
            statusbar->color(fl_rgb_color(100,200,100));
        }
        else
        {
            statusbar->color(46);
        }
        statusbar->copy_label(displayedLabel.c_str());
        if (oldLabel.length()>0) Fl::flush();
    }
}

void ConnectionHandling::condSleep(unsigned long targetSleepTimeInSec)
{
    unsigned long long targetSleepTimeInMicrSec = targetSleepTimeInSec;
    targetSleepTimeInMicrSec*=1000;
    targetSleepTimeInMicrSec*=1000;
    unsigned long long totalSleepTime=0;
    unsigned long long sleepStep = min(targetSleepTimeInMicrSec, (unsigned long long) 100000);
    do
    {
        usleep(sleepStep);
        totalSleepTime+=sleepStep;
    }
    while(reachOutRunning && totalSleepTime<targetSleepTimeInMicrSec);
}

void* ConnectionHandling::downloadRoutine(void *connectionHandling)
{
    ConnectionHandling* connection=(ConnectionHandling*) connectionHandling;
    connection->downloadStopped = false;

    // check url
    size_t urlLength = connection->nonNotarySourceUrl.length();
    const bool goodUrl = (urlLength > 0
                          && connection->nonNotarySourceUrl.substr(urlLength-1, 1).compare("/") == 0);

    // try to download type1entry
    Type1Entry *type1entry = connection->rqstBuilder->getKeysHandling()->getType1Entry();
    if (goodUrl && type1entry==nullptr && connection->downloadRunning)
    {
        string byteSequence;
        string url(connection->nonNotarySourceUrl);
        url.append("entry.t1e");
        if (downloadFile(url, byteSequence))
        {
            Type1Entry t1e;
            if (t1e.createFromString(byteSequence) && t1e.isGood())
            {
                // save and load
                string fileName("conf/type1entry/entry.t1e");
                t1e.saveToFile(fileName);
                connection->rqstBuilder->getKeysHandling()->loadType1Entry();
                type1entry = connection->rqstBuilder->getKeysHandling()->getType1Entry();
                if (type1entry!=nullptr) connection->loadDataFromConnectionsFile();
            }
            else connection->logError("could not create good Type1Entry");
        }
    }

    // try to download some contact
    connection->connection_mutex.lock();
    size_t knownContacts = connection->servers.size();
    connection->connection_mutex.unlock();
    for (int i=0; i<5 && goodUrl && type1entry!=nullptr &&
            (knownContacts<2 || checkForNewContacts>0) && connection->downloadRunning; i++)
    {
        string byteSequence;
        string url(connection->nonNotarySourceUrl);
        url.append("ContactInfo");
        if (i>0) url.append(to_string(i));
        url.append(".ci");
        if (downloadFile(url, byteSequence))
        {
            ContactInfo ci(byteSequence);
            if (ci.getIP().length()>3) connection->tryToStoreContactInfo(ci, false);
        }
        connection->connection_mutex.lock();
        knownContacts = connection->servers.size();
        connection->connection_mutex.unlock();
    }

    // try to download some liqui IDs
    connection->rqstBuilder->getLiquiditiesHandling()->liquis_mutex.lock();
    size_t numOfRegisteredLiquis = connection->rqstBuilder->getLiquiditiesHandling()->numOfRegistered();
    connection->rqstBuilder->getLiquiditiesHandling()->liquis_mutex.unlock();
    if (goodUrl && type1entry!=nullptr && connection->downloadRunning
            && (numOfRegisteredLiquis==0 || checkForNewLiquis>0))
    {
        string byteSequence;
        string url(connection->nonNotarySourceUrl);
        url.append("liquisIDs.json");
        if (downloadFile(url, byteSequence))
        {
            connection->rqstBuilder->getLiquiditiesHandling()->liquis_mutex.lock();
            if (connection->rqstBuilder->getLiquiditiesHandling()->numOfRegistered()==0 || checkForNewLiquis>0)
            {
                ofstream outfile("conf/liquisIDs_download.json", ios::binary);
                outfile << byteSequence;
                outfile.close();
            }
            connection->rqstBuilder->getLiquiditiesHandling()->liquis_mutex.unlock();
            connection->rqstBuilder->getLiquiditiesHandling()->loadIDsFromFile(true);
            connection->rqstBuilder->getLiquiditiesHandling()->saveIDs();
        }
    }

    connection->downloadStopped=true;
    return NULL;
}

void* ConnectionHandling::reachOutRoutine(void *connectionHandling)
{
    ConnectionHandling* connection=(ConnectionHandling*) connectionHandling;
    connection->reachOutStopped=false;
    do
    {
        // check if already connected
        if (connection->connectionEstablished())
        {
            connection->connection_mutex.lock();
            unsigned long notaryNr = connection->connectedNotary;
            if (notaryNr>0)
            {
                string connectedText("Connected to notary ");
                connectedText.append(to_string(notaryNr));
                connection->displayStatus(connectedText.c_str(), true);
            }
            connection->connection_mutex.unlock();
            connection->condSleep(reachOutRoutineSleepTime);
            continue;
        }
        else
        {
            // delete buffer
            connection->requests_buffer_mutex.lock();
            connection->requestsStr.clear();
            connection->requests_buffer_mutex.unlock();
        }

        // check if any servers are known
        connection->connection_mutex.lock();
        if (connection->servers.size()==0)
        {
            connection->displayStatus("Looking for connection data...", false);
            connection->connection_mutex.unlock();
            connection->condSleep(reachOutRoutineSleepTime);
            continue;
        }
        // try to connect to randomly chosen server
        connection->displayStatus("Connecting...", false);
        unsigned long eligibleNotary = connection->getSomeEligibleNotary();
        if (eligibleNotary>0)
        {
            string reportTxt("trying to connect to ");
            reportTxt.append(to_string(eligibleNotary));
            connection->logInfo(reportTxt.c_str());
            // reconnect
            HandlerNotaryPair* hnpair = new HandlerNotaryPair(connection, eligibleNotary);
            pthread_t connectToThread;
            if(pthread_create(&connectToThread, NULL, connectToNotaryRoutine, (void*) hnpair) >= 0)
            {
                pthread_detach(connectToThread);
            }
        }
        connection->connection_mutex.unlock();
        connection->condSleep(reachOutRoutineSleepTime);
    }
    while(connection->reachOutRunning);
    connection->reachOutStopped=true;
    return NULL;
}

// mutex must be locked before start of this (and is locked after):
unsigned long ConnectionHandling::getSomeEligibleNotary()
{
    if (!reachOutRunning || servers.size()==0) return 0;
    Type1Entry *type1entry = rqstBuilder->getKeysHandling()->getType1Entry();
    srand((unsigned int)(systemTimeInMs() % 10000));
    for (unsigned char c=0; c<10; c++)
    {
        auto it = servers.begin();
        advance(it, rand() % servers.size());
        unsigned long out = it->first;
        unsigned long long currentTime = systemTimeInMs();
        if (type1entry->isActingNotary(out, currentTime)
                && (notariesBans.count(out)<=0 || notariesBans[out]<=currentTime)) return out;
    }
    return 0;
}

void *ConnectionHandling::connectToNotaryRoutine(void *handlerNotaryPair)
{
    HandlerNotaryPair* hNPair=(HandlerNotaryPair*) handlerNotaryPair;
    ConnectionHandling* cHandler = hNPair->cHandling;
    const unsigned long notary = hNPair->notary;
    delete hNPair;

    cHandler->connection_mutex.lock();

    if (!cHandler->reachOutRunning)
    {
        cHandler->connection_mutex.unlock();
        return NULL;
    }

    if (cHandler->servers.count(notary)!=1)
    {
        cHandler->connection_mutex.unlock();
        return NULL;
    }
    ContactInfo* ci=cHandler->servers[notary];

    // exit if connection not (fully) stopped
    if ((cHandler->socketNr!=-1 && cHandler->listenOnSocket)
            || !cHandler->socketReaderStopped)
    {
        cHandler->connection_mutex.unlock();
        return NULL;
    }

    const string ip = ci->getIP();
    const int port = ci->getPort();

    cHandler->socketNrInAttempt=-1;
    cHandler->socketReaderStopped=false;
    cHandler->connection_mutex.unlock(); // unlock for the connection build up
    usleep(connectionTimeOutCheckInMs * 3 * 1000);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        cHandler->connection_mutex.lock();
        cHandler->socketReaderStopped=true;
        goto fail;
    }

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // start attemptInterruptionThread
    cHandler->socketNrInAttempt=sock;
    if(pthread_create(&cHandler->attemptInterruptionThread, NULL, attemptInterrupter, (void*) cHandler) < 0)
    {
        cHandler->logError("ConnectionHandling:: failed to create attemptInterruptionThread");
        cHandler->connection_mutex.lock();
        cHandler->socketReaderStopped=true;
        goto fail;
    }
    pthread_detach(cHandler->attemptInterruptionThread);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cHandler->connection_mutex.lock();
        cHandler->socketNrInAttempt=-1;
        cHandler->socketReaderStopped=true;
        goto fail;
    }
    else // connection established
    {
        cHandler->connection_mutex.lock();
        cHandler->socketNrInAttempt=-1;

        // set connection data
        cHandler->socketNr=sock;
        cHandler->listenOnSocket=true;
        cHandler->msgBuilder = new MessageBuilder(((MessageProcessor*)cHandler->msgProcessor));
        cHandler->msgBuilder->setLogger(cHandler->log);
        if(pthread_create(&cHandler->listenerThread, NULL, socketReader, (void*) cHandler) < 0)
        {
            cHandler->listenOnSocket=false;
            closeconnection(cHandler->socketNr);
            delete cHandler->msgBuilder;
            cHandler->msgBuilder=nullptr;
            cHandler->socketNr=-1;
            cHandler->socketReaderStopped=true;
            cHandler->connection_mutex.unlock();
            return NULL;
        }
        pthread_detach(cHandler->listenerThread);
        cHandler->connectedNotary = notary;

        // send outstanding messages + flush buffer:
        cHandler->requests_buffer_mutex.lock();
        // append connection close msg
        string rqst;
        byte type = 20;
        rqst.push_back((char)type);
        cHandler->rqstBuilder->packRequest(&rqst);
        cHandler->requestsStr.append(rqst);
        // send and exit
        send(cHandler->socketNr, cHandler->requestsStr.c_str(), cHandler->requestsStr.length(), MSG_NOSIGNAL);
        cHandler->requestsStr.clear();
        cHandler->requests_buffer_mutex.unlock();

        cHandler->connection_mutex.unlock();
        return NULL;
    }
fail:
    string reportTxt("ConnectionHandling:: failed to connect to socket ");
    reportTxt.append(to_string(sock));
    cHandler->logError(reportTxt.c_str());
    closeconnection(sock);
    cHandler->connection_mutex.unlock();
    return NULL;
}

void* ConnectionHandling::attemptInterrupter(void *connectionHandling)
{
    ConnectionHandling* connection = (ConnectionHandling*) connectionHandling;
    connection->attemptInterrupterStopped = false;
    int sock = connection->socketNrInAttempt;
    if (sock == -1) goto close;
    for(int counter = 0; connection->reachOutRunning
            && counter*connectionTimeOutCheckInMs < connectionTimeOutInMs; counter++)
    {
        usleep(connectionTimeOutCheckInMs * 1000);
        if (sock != connection->socketNrInAttempt)
        {
            goto close;
        }
    }
    connection->closeconnection(sock); // timeout
close:
    connection->attemptInterrupterStopped = true;
    return NULL;
}

void* ConnectionHandling::socketReader(void *connectionHandling)
{
    ConnectionHandling* connection = (ConnectionHandling*) connectionHandling;
    connection->socketReaderStopped = false;
    connection->lastConnectionTime = systemTimeInMs();
    connection->lastListeningTime = connection->lastConnectionTime;

    int n;
    byte buffer[1024];
    while(connection->listenOnSocket)
    {
        n=recv(connection->socketNr, buffer, 1024, 0);
        // n=recv(connection->socketNr, (char *) buffer, 1024, 0); // under mingw
        if(n<=0) goto close;
        else
        {
            connection->logInfo("ConnectionHandling::socketReader: incoming message, length:");
            connection->logInfo(to_string(n).c_str());
            for (int i=0; i<n; i++)
            {
                if (!connection->listenOnSocket
                        || !connection->msgBuilder->addByte(buffer[i])) goto close;
            }
        }
    }
close:
    connection->lastListeningTime=systemTimeInMs();
    connection->listenOnSocket=false;
    closeconnection(connection->socketNr);
    connection->socketNr=-1;
    delete connection->msgBuilder;
    connection->msgBuilder=nullptr;
    connection->socketReaderStopped=true;
    return NULL;
}

void ConnectionHandling::addRequest(string &rqst)
{
    requests_buffer_mutex.lock();
    requestsStr.append(rqst);
    requests_buffer_mutex.unlock();
}

size_t ConnectionHandling::requestsStrLength()
{
    requests_buffer_mutex.lock();
    size_t out = requestsStr.length();
    requests_buffer_mutex.unlock();
    return out;
}

unsigned long long ConnectionHandling::systemTimeInMs()
{
    using namespace std::chrono;
    milliseconds ms = duration_cast< milliseconds >(
                          system_clock::now().time_since_epoch()
                      );
    return ms.count();
}

bool ConnectionHandling::downloadFile(string &url, string &output)
{
    if (output.length()>0) return false;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    stringstream out;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ConnectionHandling::write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    if (curl_easy_perform(curl) != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        return false;
    }
    output = out.str();
    curl_easy_cleanup(curl);
    return true;
}

size_t ConnectionHandling::write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    string data((const char*) ptr, (size_t) size * nmemb);
    *((stringstream*) stream) << data << endl;
    return size * nmemb;
}

void ConnectionHandling::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void ConnectionHandling::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}

ConnectionHandling::HandlerNotaryPair::HandlerNotaryPair(ConnectionHandling* c, unsigned long n) : cHandling(c), notary(n)
{
}

