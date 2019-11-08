#include "InternalThread.h"

#define routineSleepTimeInSec 1
#define collectInfoFreq 8
#define claimsInfoFreq 25
#define idsInfoFreq 45
#define contactsRqstFreq 55
#define heartBeatFreq 15
#define keyRegistrationFreq 5
#define liquiRegistrationFreq 5

InternalThread::InternalThread(ConnectionHandling *c): connection(c), log(nullptr),
    collectInfoCount(0), claimsInfoCount(0), idsInfoCount(0), contactsRqstCount(0), heartBeatCount(0)
{
    //ctor
}

InternalThread::~InternalThread()
{
    //dtor
}

void InternalThread::setLogger(Logger *l)
{
    log=l;
}

void InternalThread::start()
{
    running=true;
    if(pthread_create(&thread, NULL, InternalThread::routine, (void*) this) < 0)
    {
        logError("could not create internal thread");
        running=false;
        return;
    }
    stopped=false;
    pthread_detach(thread);
}

void InternalThread::condSleep(unsigned long targetSleepTimeInSec)
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
    while(running && totalSleepTime<targetSleepTimeInMicrSec);
}

void* InternalThread::routine(void *internalThread)
{
    InternalThread* internal=(InternalThread*) internalThread;
    internal->stopped=false;
    internal->logInfo("InternalThread::routine started");
    internal->heartBeatCount = heartBeatFreq;
    internal->contactsRqstCount = contactsRqstFreq/5*4;
    internal->keyRegistrationCount = keyRegistrationFreq;
    internal->liquiRegistrationCount = liquiRegistrationFreq;
    do
    {
        internal->condSleep(routineSleepTimeInSec);

        if (!internal->running || internal->connection->getNotaryNr()<1
                || !internal->connection->connectionEstablished())
        {
            internal->heartBeatCount = heartBeatFreq;
            internal->contactsRqstCount = contactsRqstFreq/5*4;
            continue;
        }

        RequestBuilder *rqstBuilder = internal->connection->getRqstBuilder();
        KeysHandling *keys = rqstBuilder->getKeysHandling();
        LiquiditiesHandling *lh = rqstBuilder->getLiquiditiesHandling();

        // register new keys if necessary
        string keyName = keys->getLocalPrvtKeyUnreg();
        if (keyName.length()>0 && internal->keyRegistrationCount >= keyRegistrationFreq)
        {
            internal->keyRegistrationCount = 0;
            internal->logInfo("InternalThread::routine: registering new key..");
            internal->logInfo(keyName.c_str());
            CryptoPP::RSA::PrivateKey* privateKey = keys->getPrivateKey(keyName);
            string* publicKeyStr = keys->getPublicKeyStr(keyName);
            unsigned long notaryNr = internal->connection->getNotaryNr();
            if (privateKey!=nullptr && publicKeyStr!=nullptr && notaryNr>0)
            {
                string rqst;
                if (rqstBuilder->createPubKeyRegRqst(*publicKeyStr, privateKey, notaryNr, rqst))
                {
                    internal->connection->sendRequest(rqst);
                }
            }
        }
        else if (keyName.length()>0) internal->keyRegistrationCount++;

        // request key info if necessary
        keyName = keys->getLocalPblcKeyUnreg();
        if (keyName.length()>0)
        {
            internal->logInfo("InternalThread::routine: requesting key info ..");
            internal->logInfo(keyName.c_str());
            string* publicKeyStr = keys->getPublicKeyStr(keyName);
            if (publicKeyStr!=nullptr)
            {
                string rqst;
                if (rqstBuilder->createPubKeyInfoRqst(*publicKeyStr, rqst))
                {
                    internal->connection->sendRequest(rqst);
                }
            }
        }

        // register new liqui if necessary and/or request id
        string liquiName = lh->getLiquiUnreg();
        if (liquiName.length()>0 && internal->liquiRegistrationCount >= liquiRegistrationFreq)
        {
            internal->liquiRegistrationCount = 0;
            Type5Or15Entry* entry = lh->getLiqui(liquiName);
            if (entry != nullptr)
            {
                CompleteID owner = entry->getOwnerID();
                string keyName = keys->getNameByID(owner);
                CryptoPP::RSA::PrivateKey* privateKey = keys->getPrivateKey(keyName);
                if (privateKey != nullptr)
                {
                    internal->logInfo("InternalThread::routine: registering new liqui ..");
                    unsigned long notaryNr = internal->connection->getNotaryNr();
                    if (notaryNr>0)
                    {
                        string rqst;
                        if (rqstBuilder->createLiquiRegRqst(entry, privateKey, owner, notaryNr, rqst))
                        {
                            internal->logInfo("InternalThread::routine: sending liqui reg request..");
                            internal->connection->sendRequest(rqst);
                        }
                    }
                }
                internal->logInfo("InternalThread::routine: requesting liqui info ..");
                string rqst;
                if (rqstBuilder->createLiquiInfoRqst(entry, rqst))
                {
                    internal->connection->sendRequest(rqst);
                }
            }
        }
        else if (liquiName.length()>0) internal->liquiRegistrationCount++;

        // request heart beat
        if (internal->heartBeatCount >= heartBeatFreq)
        {
            string rqst;
            if (rqstBuilder->createHeartBeatRqst(rqst) && internal->connection->connectionEstablished())
            {
                internal->connection->sendRequest(rqst);
                internal->heartBeatCount=0;
            }
        }
        else internal->heartBeatCount++;

        // request contacts
        if (internal->contactsRqstCount >= contactsRqstFreq)
        {
            string rqst;
            if (rqstBuilder->createContactsRqst(rqst) && internal->connection->connectionEstablished())
            {
                internal->connection->sendRequest(rqst);
                internal->contactsRqstCount=0;
            }
        }
        else internal->contactsRqstCount++;

        // request recent claims and appointment info
        internal->ofinterest_mutex.lock();
        if (internal->claimsInfoCount >= claimsInfoFreq)
        {
            set<pair<string,string>>::iterator it;
            for (it=internal->accountPairsOfInterest.begin(); it!=internal->accountPairsOfInterest.end(); ++it)
            {
                string rqst;
                CompleteID largestID(ULONG_MAX, ULLONG_MAX, ULLONG_MAX);
                string keyName = it->first;
                string liquiName = it->second;
                if (rqstBuilder->createClaimsInfoRqst(keyName, liquiName, largestID, 2, rqst))
                {
                    internal->logInfo("InternalThread::routine: sending claims Info Request..");
                    internal->connection->sendRequest(rqst);
                }
                rqst="";
                if (rqstBuilder->createRefInfoRqst(keyName, liquiName, rqst))
                {
                    internal->logInfo("InternalThread::routine: sending ref Info Request..");
                    internal->connection->sendRequest(rqst);
                }
            }
            internal->claimsInfoCount=0;
        }
        else internal->claimsInfoCount++;
        internal->ofinterest_mutex.unlock();

        // check if new claims are to be made
        internal->ofinterest_mutex.lock();
        if (internal->collectInfoCount >= collectInfoFreq)
        {
            set<pair<string,string>>::iterator it;
            for (it=internal->accountPairsOfInterest.begin(); it!=internal->accountPairsOfInterest.end(); ++it)
            {
                string rqst;
                string keyName = it->first;
                string liquiName = it->second;
                if (keys->getPrivateKey(keyName)!=nullptr && rqstBuilder->createNextClaimRqst(keyName, liquiName, rqst))
                {
                    internal->logInfo("InternalThread::routine: sending Next Claim Request..");
                    internal->connection->sendRequest(rqst);
                }
            }
            internal->collectInfoCount=0;
        }
        else internal->collectInfoCount++;
        internal->ofinterest_mutex.unlock();

        // check for updates on ids of interest
        internal->ofinterest_mutex.lock();
        if (internal->idsInfoCount >= idsInfoFreq)
        {
            // augment ids of interest
            list<string> keysNames;
            keys->loadPublicKeysNames(keysNames);
            list<string>::iterator namesIt;
            for (namesIt=keysNames.begin(); namesIt!=keysNames.end(); ++namesIt)
            {
                internal->idsOfInterest.insert(keys->getInitialID(*namesIt));
            }
            list<string> liquisNames;
            lh->loadKnownLiquidities(liquisNames);
            for (namesIt=liquisNames.begin(); namesIt!=liquisNames.end(); ++namesIt)
            {
                internal->idsOfInterest.insert(lh->getID(*namesIt));
            }
            // run through ids of interest
            set<CompleteID, CompleteID::CompareIDs>::iterator it;
            for (it=internal->idsOfInterest.begin(); it!=internal->idsOfInterest.end(); ++it)
            {
                // check if update makes sense
                string liquiName = lh->getNameById(*it);
                if (lh->getLiqui(liquiName)!=nullptr) continue;
                // create request
                string rqst;
                if (rqstBuilder->createIdInfoRqst(*it, rqst))
                {
                    internal->logInfo("InternalThread::routine: sending Id Info Request..");
                    internal->connection->sendRequest(rqst);
                }
            }
            internal->idsInfoCount=0;
        }
        else internal->idsInfoCount++;
        internal->ofinterest_mutex.unlock();
    }
    while(internal->running);
    internal->logInfo("InternalThread::routine stopped");
    internal->stopped=true;
    return NULL;
}

void InternalThread::addAccountPairOfInterest(string key, string liqui)
{
    if(connection->getNotaryNr()>0)
    {
        string rqst;
        CompleteID largestID(ULONG_MAX, ULLONG_MAX, ULLONG_MAX);
        RequestBuilder *rqstBuilder = connection->getRqstBuilder();
        if (rqstBuilder->createClaimsInfoRqst(key, liqui, largestID, 2, rqst))
        {
            logInfo("InternalThread::addAccountPairOfInterest: sending claims Info Request..");
            connection->sendRequest(rqst);
        }
        rqst="";
        if (rqstBuilder->createRefInfoRqst(key, liqui, rqst))
        {
            logInfo("InternalThread::addAccountPairOfInterest: sending ref Info Request..");
            connection->sendRequest(rqst);
        }
        rqst="";
        if (rqstBuilder->getKeysHandling()->getPrivateKey(key) != nullptr
                && rqstBuilder->createNextClaimRqst(key, liqui, rqst))
        {
            logInfo("InternalThread::addAccountPairOfInterest: sending Next Claim Request..");
            connection->sendRequest(rqst);
        }
    }
    pair<string,string> accountPair(key, liqui);
    ofinterest_mutex.lock();
    if (accountPairsOfInterest.count(accountPair)!=0)
    {
        logInfo("InternalThread::addAccountPairOfInterest: not new pair..");
        ofinterest_mutex.unlock();
        logInfo("InternalThread::addAccountPairOfInterest: exit..");
        return;
    }
    logInfo("InternalThread::addAccountPairOfInterest: adding new pair..");
    accountPairsOfInterest.insert(accountPair);
    ofinterest_mutex.unlock();
}

ConnectionHandling* InternalThread::getConnectionHandling()
{
    return connection;
}

void InternalThread::addIdOfInterest(CompleteID id)
{
    logInfo("InternalThread::addIdOfInterest: ");
    logInfo(id.toHex().c_str());
    if (id.getNotary()<=0)
    {
        logError("InternalThread::addIdOfInterest: bad id");
        return;
    }

    ofinterest_mutex.lock();
    if(connection->getNotaryNr()>0)
    {
        RequestBuilder *requestBuilder = connection->getRqstBuilder();
        // check if update makes sense
        LiquiditiesHandling *lh = requestBuilder->getLiquiditiesHandling();
        string liquiName = lh->getNameById(id);
        if (lh->getLiqui(liquiName)==nullptr && requestBuilder->getKeysHandling()->getNameByID(id).length()<1)
        {
            // create request
            string rqst;
            if (requestBuilder->createIdInfoRqst(id, rqst))
            {
                logInfo("InternalThread::addIdOfInterest: sending Id Info Request..");
                connection->sendRequest(rqst);
            }
        }
    }
    if (idsOfInterest.count(id)!=0)
    {
        ofinterest_mutex.unlock();
        return;
    }
    idsOfInterest.insert(id);
    ofinterest_mutex.unlock();
}

void InternalThread::stopSafely()
{
    running=false;
    while (!stopped)
    {
        logInfo("InternalThread::stopSafely: !stopped");
        usleep(100000);
    }
}

void InternalThread::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void InternalThread::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}
