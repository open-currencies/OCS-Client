#include "MessageProcessor.h"
#include "ConnectionHandling.h"

#define maxAnticipations 10

MessageProcessor::MessageProcessor() : ch(nullptr), log(nullptr), nextRqstNum(1)
{
    //ctor
}

MessageProcessor::~MessageProcessor()
{
    map<unsigned long, list<pair<Type12Entry*, CIDsSet*>>*>::iterator it;
    for (it=processedRqsts.begin(); it!=processedRqsts.end(); ++it)
    {
        deleteList(it->second);
    }
    processedRqsts.clear();

    map<unsigned long, string*>::iterator it2;
    for (it2=processedRqstsStr.begin(); it2!=processedRqstsStr.end(); ++it2)
    {
        delete it2->second;
    }
    processedRqstsStr.clear();
}

void MessageProcessor::deleteList(list<pair<Type12Entry*, CIDsSet*>>* elem)
{
    if (elem==nullptr) return;
    list<pair<Type12Entry*, CIDsSet*>>::iterator it;
    for (it=elem->begin(); it!=elem->end(); ++it)
    {
        delete it->first;
        it->second->clear();
        delete it->second;
    }
    elem->clear();
    delete elem;
}

unsigned long MessageProcessor::addRqstAnticipation(string &rqst)
{
    logInfo("MessageProcessor::addRqstAnticipation start");

    rqststore_mutex.lock();
    // return num if request not new
    if (anticipatedRqstByStr.count(rqst)!=0)
    {
        unsigned long out = anticipatedRqstByStr[rqst];
        rqststore_mutex.unlock();
        return out;
    }
    // check if maps not too large and delete old rqsts if necessary
    if (anticipatedRqstByNum.size() > maxAnticipations)
    {
        logInfo("MessageProcessor::addRqstAnticipation: deleting old anticipation");
        unsigned long oldRqstNum = anticipatedRqstByNum.begin()->first;
        string oldRqst = anticipatedRqstByNum[oldRqstNum];
        anticipatedRqstByStr.erase(oldRqst);
        anticipatedRqstByNum.erase(oldRqstNum);
        // delete from processedRqsts list
        if (processedRqsts.count(oldRqstNum)>0)
        {
            list<pair<Type12Entry*, CIDsSet*>>* old = processedRqsts[oldRqstNum];
            processedRqsts.erase(oldRqstNum);
            deleteList(old);
        }
        if (processedRqstsStr.count(oldRqstNum)>0)
        {
            string* old = processedRqstsStr[oldRqstNum];
            processedRqstsStr.erase(oldRqstNum);
            delete old;
        }
    }
    anticipatedRqstByStr.insert(pair<string, unsigned long>(rqst, nextRqstNum));
    anticipatedRqstByNum.insert(pair<unsigned long, string>(nextRqstNum, rqst));
    unsigned long out = nextRqstNum;
    nextRqstNum++;
    rqststore_mutex.unlock();

    logInfo("MessageProcessor::addRqstAnticipation end");
    return out;
}

bool MessageProcessor::storeProcessedRqst(string &rqst, list<pair<Type12Entry*, CIDsSet*>>* result)
{
    rqststore_mutex.lock();
    if (anticipatedRqstByStr.count(rqst)!=1)
    {
        rqststore_mutex.unlock();
        return false;
    }
    unsigned long num = anticipatedRqstByStr[rqst];
    if (result != nullptr)
    {
        processedRqsts.insert(pair<unsigned long, list<pair<Type12Entry*, CIDsSet*>>*>(num, result));
    }
    anticipatedRqstByStr.erase(rqst);
    anticipatedRqstByNum.erase(num);
    rqststore_mutex.unlock();
    return true;
}

bool MessageProcessor::storeProcessedRqstStr(string &rqst, string* result)
{
    rqststore_mutex.lock();
    if (anticipatedRqstByStr.count(rqst)!=1)
    {
        rqststore_mutex.unlock();
        return false;
    }
    unsigned long num = anticipatedRqstByStr[rqst];
    if (result != nullptr)
    {
        processedRqstsStr.insert(pair<unsigned long, string*>(num, result));
    }
    anticipatedRqstByStr.erase(rqst);
    anticipatedRqstByNum.erase(num);
    rqststore_mutex.unlock();
    return true;
}

list<pair<Type12Entry*, CIDsSet*>>* MessageProcessor::getProcessedRqst(unsigned long rqstNum)
{
    logInfo("MessageProcessor::getProcessedRqst start");
    rqststore_mutex.lock();
    if (processedRqsts.count(rqstNum)!=1)
    {
        rqststore_mutex.unlock();
        return nullptr;
    }
    list<pair<Type12Entry*, CIDsSet*>>* out = processedRqsts[rqstNum];
    rqststore_mutex.unlock();
    logInfo("MessageProcessor::getProcessedRqst end");
    return out;
}

string* MessageProcessor::getProcessedRqstStr(unsigned long rqstNum)
{
    rqststore_mutex.lock();
    if (processedRqstsStr.count(rqstNum)!=1)
    {
        rqststore_mutex.unlock();
        return nullptr;
    }
    string* out = processedRqstsStr[rqstNum];
    rqststore_mutex.unlock();
    return out;
}

void MessageProcessor::ignoreOldRqst(unsigned long rqstNum)
{
    rqststore_mutex.lock();
    // delete from anticipation lists
    if (anticipatedRqstByNum.count(rqstNum)>0)
    {
        string rqst = anticipatedRqstByNum[rqstNum];
        anticipatedRqstByStr.erase(rqst);
        anticipatedRqstByNum.erase(rqstNum);
    }
    // delete from processedRqsts list
    if (processedRqsts.count(rqstNum)<1)
    {
        if (processedRqstsStr.count(rqstNum)>0)
        {
            processedRqstsStr.erase(rqstNum);
        }
        rqststore_mutex.unlock();
        return;
    }
    processedRqsts.erase(rqstNum);
    rqststore_mutex.unlock();
}

void MessageProcessor::deleteOldRqst(unsigned long rqstNum)
{
    logInfo("MessageProcessor::deleteOldRqst");
    rqststore_mutex.lock();
    // delete from anticipation lists
    if (anticipatedRqstByNum.count(rqstNum)>0)
    {
        string rqst = anticipatedRqstByNum[rqstNum];
        anticipatedRqstByStr.erase(rqst);
        anticipatedRqstByNum.erase(rqstNum);
    }
    // delete from processedRqsts list
    if (processedRqsts.count(rqstNum)<1)
    {
        if (processedRqstsStr.count(rqstNum)>0)
        {
            string* out = processedRqstsStr[rqstNum];
            processedRqstsStr.erase(rqstNum);
            delete out;
        }
        rqststore_mutex.unlock();
        return;
    }
    list<pair<Type12Entry*, CIDsSet*>>* out = processedRqsts[rqstNum];
    processedRqsts.erase(rqstNum);
    deleteList(out);
    rqststore_mutex.unlock();
    logInfo("MessageProcessor::deleteOldRqst done");
}

void MessageProcessor::setConnectingHandling(ConnectionHandling *c)
{
    ch=c;
}

void MessageProcessor::process(const int n, byte *message)
{
    logInfo("MessageProcessor::process: new message received");
    if (n<1) return;
    byte type=message[0];
    switch (type)
    {
    case 18:
        considerContactInfoRequest(n, message);
        break;
    case 255:
        heartBeatMsg(n, message);
        break;
    case 254:
        pblcKeyInfoMsg(n, message);
        break;
    case 253:
        currOrOblInfoMsg(n, message);
        break;
    case 252:
        idInfoMsg(n, message);
        break;
    case 251:
        claimsMsg(n, message);
        break;
    case 250:
        nextClaimMsg(n, message);
        break;
    case 249:
        transactionsMsg(n, message);
        break;
    case 248:
        decThreadsMsg(n, message);
        break;
    case 247:
        refInfoMsg(n, message);
        break;
    case 246:
        essentialsMsg(n, message);
        break;
    case 245:
        notaryInfoMsg(n, message);
        break;
    case 244:
        isBannedMsg(n, message);
        break;
    default:
        return;
    }
}

void MessageProcessor::isBannedMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::isBannedMsg");
    ch->banCurrentNotary();
}

void MessageProcessor::heartBeatMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::heartBeatMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    if (str.length()!=8) return;
    unsigned long long serverTime = u.byteSeqAsUll(str);
    ch->getRqstBuilder()->setCurrentTimeInMs(serverTime);
    // build and send essentials request
    RequestBuilder* rqstBuilder = ch->getRqstBuilder();
    rqstBuilder->getKeysHandling()->lock();
    CompleteID lastT2EId = rqstBuilder->getKeysHandling()->getLastT2eIdFirst();
    rqstBuilder->getKeysHandling()->unlock();
    string rqst;
    bool result = rqstBuilder->createEssentialsRqst(lastT2EId, rqst);
    if (result) ch->sendRequest(rqst);
}

void MessageProcessor::idInfoMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::idInfoMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length()) return;
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::idInfoMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract entries list
    string t13eList = str.substr(i, str.length()-i);
    list<pair<Type12Entry*, CIDsSet*>>* entriesList = extractType12Entries(t13eList, false);
    // some processing based on type
    if (entriesList!=nullptr && entriesList->size()>0)
    {
        pair<Type12Entry*, CIDsSet*> *firstPair = &entriesList->front();
        RequestBuilder* rqstBuilder = ch->getRqstBuilder();
        int type = firstPair->first->underlyingType();
        if (type == 5 || type == 15)
        {
            Type5Or15Entry* t5Or15e = (Type5Or15Entry*) firstPair->first->underlyingEntry()->createCopy();
            CompleteID liquiID = firstPair->second->last();
            string liqui = rqstBuilder->getLiquiditiesHandling()->getNameById(liquiID);
            logInfo("MessageProcessor::idInfoMsg: setting t5Or15e");
            if (!rqstBuilder->getLiquiditiesHandling()->setLiqui(liqui, t5Or15e))
            {
                logError("MessageProcessor::idInfoMsg: setting t5Or15e failed");
                delete t5Or15e;
            }
        }
        else if (type == 11)
        {
            // determine name (local), validity and aliases
            string name;
            Type11Entry* t11e = nullptr;
            list<CompleteID> allRelevantIDs;
            unsigned long long minValidity = ULLONG_MAX;
            list<pair<Type12Entry*, CIDsSet*>>::iterator it;
            for (it=entriesList->begin(); it!=entriesList->end(); ++it)
            {
                t11e = (Type11Entry*) it->first->underlyingEntry()->createCopy();
                minValidity = min(minValidity, t11e->getValidityDate());
                if (allRelevantIDs.empty())
                {
                    set<CompleteID, CompleteID::CompareIDs>* idsList = it->second->getSetPointer();
                    set<CompleteID, CompleteID::CompareIDs>::iterator it2;
                    for (it2=idsList->begin(); it2!=idsList->end(); ++it2)
                    {
                        CompleteID id = *it2;
                        allRelevantIDs.push_back(id);
                        if (name.length()==0)
                        {
                            name = rqstBuilder->getKeysHandling()->getNameByID(id);
                        }
                    }
                    if (name.length()==0) allRelevantIDs.clear();
                }
            }
            if (name.length()>0 && t11e!=nullptr)
            {
                logInfo("MessageProcessor::idInfoMsg: saving to KeysHandling");
                // save to KeysHandling
                bool addedIds=false;
                rqstBuilder->getKeysHandling()->updateValidity(name, minValidity);
                list<CompleteID>::iterator it2;
                for (it2=allRelevantIDs.begin(); it2!=allRelevantIDs.end(); ++it2)
                {
                    addedIds = rqstBuilder->getKeysHandling()->addIdAlias(name, *it2) || addedIds;
                }
                if (addedIds)
                {
                    rqstBuilder->getKeysHandling()->savePublicKeysIDs();
                }
                // storing public key string
                rqstBuilder->getKeysHandling()->tryToStorePublicKey(name, *t11e->getPublicKey());
                delete t11e;
            }
            else
            {
                logError("MessageProcessor::idInfoMsg: no key id found");
            }
        }
    }
    // store as processed request
    bool stored=false;
    if (entriesList!=nullptr)
    {
        stored = storeProcessedRqst(rqst, entriesList);
    }
    else
    {
        logError("MessageProcessor::idInfoMsg: entriesList==nullptr");
        return;
    }
    if (entriesList->size()==0)
    {
        logError("MessageProcessor::idInfoMsg: entriesList is empty");
        if (!stored) deleteList(entriesList);
        return;
    }
    if (!stored) deleteList(entriesList);
}

void MessageProcessor::claimsMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::claimsMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length())
    {
        logError("MessageProcessor::claimsMsg: bad format 1");
        return;
    }
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::claimsMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract claims list
    string t13eList = str.substr(i, str.length()-i);
    list<pair<Type12Entry*, CIDsSet*>>* claimsList = extractType12Entries(t13eList, false);
    if (claimsList==nullptr)
    {
        logError("MessageProcessor::claimsMsg: claimsList==nullptr");
        return;
    }
    // set latest claim to request builder
    if (claimsList->size() > 0)
    {
        RequestBuilder* rqstBuilder = ch->getRqstBuilder();
        pair<Type12Entry*, CIDsSet*> *lastPair = &claimsList->back();
        Type14Entry* t14e = (Type14Entry*) lastPair->first->underlyingEntry()->createCopy();
        CompleteID ownerID = t14e->getOwnerID();
        CompleteID liquiID = t14e->getCurrencyOrObligation();
        string key = rqstBuilder->getKeysHandling()->getNameByID(ownerID);
        string liqui = rqstBuilder->getLiquiditiesHandling()->getNameById(liquiID);
        CIDsSet *idsSet = lastPair->second->createCopy();
        if (!ch->getRqstBuilder()->setLatestClaim(key, liqui, t14e, idsSet))
        {
            delete t14e;
        }
    }
    else
    {
        logError("MessageProcessor::claimsMsg: no claims reported");
    }
    // store as processed request
    bool stored = storeProcessedRqst(rqst, claimsList);
    if (!stored) deleteList(claimsList);
}

void MessageProcessor::notaryInfoMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::notaryInfoMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length()) return;
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::notaryInfoMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract notary info
    string *notaryInfoStr = new string(str.substr(i, str.length()-i));
    // set notary info in rqstBuilder
    RequestBuilder* rqstBuilder = ch->getRqstBuilder();
    Util u;
    if (9 > rqst.length())
    {
        logError("MessageProcessor::notaryInfoMsg: bad format 3");
        bool stored = storeProcessedRqstStr(rqst, notaryInfoStr);
        if (!stored) delete notaryInfoStr;
        return;
    }
    string pubKeyLenStr = rqst.substr(5, 4);
    unsigned short pubKeyLen = u.byteSeqAsUs(pubKeyLenStr);
    if ((size_t) pubKeyLen+9 > rqst.length())
    {
        logError("MessageProcessor::notaryInfoMsg: bad format 4");
        bool stored = storeProcessedRqstStr(rqst, notaryInfoStr);
        if (!stored) delete notaryInfoStr;
        return;
    }
    string pubKey = rqst.substr(9, pubKeyLen);
    string keyName = rqstBuilder->getKeysHandling()->getName(pubKey);
    if (keyName.length()>0)
    {
        // build NotaryInfo
        NotaryInfo* notaryInfo = new NotaryInfo(*notaryInfoStr);
        ch->getRqstBuilder()->setNotaryInfo(keyName, notaryInfo);
    }
    // store as processed request
    bool stored = storeProcessedRqstStr(rqst, notaryInfoStr);
    if (!stored) delete notaryInfoStr;
}

void MessageProcessor::refInfoMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::refInfoMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length()) return;
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::refInfoMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract ref info
    string *refInfoStr = new string(str.substr(i, str.length()-i));
    // set referee info in rqstBuilder
    RequestBuilder* rqstBuilder = ch->getRqstBuilder();
    Util u;
    if (45 > rqst.length())
    {
        logError("MessageProcessor::refInfoMsg: bad format 3");
        bool stored = storeProcessedRqstStr(rqst, refInfoStr);
        if (!stored) delete refInfoStr;
        return;
    }
    string dum = rqst.substr(5, 20);
    CompleteID refID(dum);
    string key = rqstBuilder->getKeysHandling()->getNameByID(refID);
    dum = rqst.substr(25, 20);
    CompleteID liquiID(dum);
    string liqui = rqstBuilder->getLiquiditiesHandling()->getNameById(liquiID);
    if (key.length()>0 && liqui.length()>0)
    {
        // build RefInfo
        RefereeInfo* refInfo = new RefereeInfo(*refInfoStr);
        ch->getRqstBuilder()->setRefInfo(key, liqui, refInfo);
    }
    // store as processed request
    bool stored = storeProcessedRqstStr(rqst, refInfoStr);
    if (!stored) delete refInfoStr;
}

void MessageProcessor::decThreadsMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::decThreadsMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length()) return;
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::decThreadsMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract entries list
    string t13eList = str.substr(i, str.length()-i);
    list<pair<Type12Entry*, CIDsSet*>>* entriesList = extractType12Entries(t13eList, false);
    // store as processed request
    bool stored=false;
    if (entriesList!=nullptr)
    {
        stored = storeProcessedRqst(rqst, entriesList);
    }
    else
    {
        logError("MessageProcessor::decThreadsMsg: entriesList==nullptr");
        return;
    }
    if (entriesList->size()==0)
    {
        logInfo("MessageProcessor::decThreadsMsg: entriesList is empty");
        if (!stored) deleteList(entriesList);
        return;
    }
    if (!stored) deleteList(entriesList);
}

void MessageProcessor::essentialsMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::essentialsMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length())
    {
        logError("MessageProcessor::essentialsMsg: bad format 01");
        return;
    }
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::essentialsMsg: bad format 02");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract and check entries
    string t13eList = str.substr(i, str.length()-i);
    KeysHandling *keys = ch->getRqstBuilder()->getKeysHandling();

    logInfo("MessageProcessor::essentialsMsg keys->lock");
    keys->lock();
    CompleteID lastT2EId = keys->getLastT2eIdFirst();
    bool consistent = false;
    CIDsSet *runningT2EId = nullptr;
    i = 0;
    while (i<t13eList.length())
    {
        // get sublist string
        if (i+8 > t13eList.length())
        {
            logError("MessageProcessor::essentialsMsg: bad format 1");
            consistent=false;
            break;
        }
        string lenOfSublistStr = t13eList.substr(i, 8);
        i+=8;
        unsigned long long lenOfSublist = u.byteSeqAsUll(lenOfSublistStr);
        if (i+lenOfSublist > t13eList.length())
        {
            logError("MessageProcessor::essentialsMsg: bad format 2");
            consistent=false;
            break;
        }
        string sublistSeq = t13eList.substr(i, lenOfSublist);
        i+=lenOfSublist;
        // get t12e and its first id
        CIDsSet* firstIDs = new CIDsSet();
        Type12Entry* t12e = extractType12Entry(sublistSeq, *firstIDs, true, true);
        if (t12e == nullptr || firstIDs->size()<=0)
        {
            logError("MessageProcessor::essentialsMsg: extractType12Entry failed 1");
            consistent=false;
            if (t12e != nullptr) delete t12e;
            delete firstIDs;
            break;
        }
        // check that Type2Entry is good
        Type2Entry *t2e = (Type2Entry*) t12e->underlyingEntry();
        if (runningT2EId!=nullptr && !runningT2EId->contains(t2e->getPredecessorID()))
        {
            consistent=false;
            delete t12e;
            delete firstIDs;
            break;
        }
        // store or continue
        if (consistent || (runningT2EId == nullptr && lastT2EId.isZero() && t2e->getPredecessorID().isZero()))
        {
            logInfo("MessageProcessor::essentialsMsg keys->addNotaryKeyEtc");
            bool result = keys->addNotaryKeyEtc(*t2e->getPublicKey(), firstIDs->first(), t2e->getTerminationID());
            if (!result) logError("MessageProcessor::essentialsMsg keys->addNotaryKeyEtc failed");
            consistent=true;
        }
        else
        {
            if (firstIDs->first() == lastT2EId) consistent = true;
        }
        // now try to build complete firstIDs
        delete firstIDs;
        delete t12e;
        firstIDs = new CIDsSet();
        t12e = extractType12Entry(sublistSeq, *firstIDs, true, false);
        if (t12e == nullptr || firstIDs->size()<=0)
        {
            logError("MessageProcessor::essentialsMsg: extractType12Entry failed 2");
            consistent=false;
            if (t12e != nullptr) delete t12e;
            delete firstIDs;
            break;
        }
        // update runningT2EId
        if (runningT2EId!=nullptr) delete runningT2EId;
        runningT2EId = firstIDs;
        delete t12e;
    }
    if (runningT2EId!=nullptr) delete runningT2EId;
    // extract entries list
    list<pair<Type12Entry*, CIDsSet*>>* entriesList = extractType12Entries(t13eList, true);
    // setLatestT2EId
    if (entriesList!=nullptr && entriesList->size()>0 && consistent)
    {
        logInfo("MessageProcessor::essentialsMsg keys->setLatestT2eId");
        keys->setLatestT2eId(entriesList->rbegin()->second->last());
    }
    keys->unlock();
    logInfo("MessageProcessor::essentialsMsg keys->unlock");

    // store as process request
    bool stored=false;
    if (entriesList!=nullptr)
    {
        stored = storeProcessedRqst(rqst, entriesList);
        logInfo("MessageProcessor::essentialsMsg storeProcessedRqst attempted");
    }
    else
    {
        logError("MessageProcessor::essentialsMsg: entriesList==nullptr");
        return;
    }
    if (entriesList->size()==0)
    {
        logError("MessageProcessor::essentialsMsg: entriesList is empty");
        if (!stored) deleteList(entriesList);
        return;
    }
    // clean up and exit
    if (!stored) deleteList(entriesList);
    logInfo("MessageProcessor::essentialsMsg exiting");
}

void MessageProcessor::transactionsMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::transactionsMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length())
    {
        logError("MessageProcessor::transactionsMsg: bad format 1");
        return;
    }
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::transactionsMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract transactions list
    string t13eList = str.substr(i, str.length()-i);
    list<pair<Type12Entry*, CIDsSet*>>* transactionsList = extractType12Entries(t13eList, false);
    // store as process request
    bool stored=false;
    if (transactionsList!=nullptr)
    {
        stored = storeProcessedRqst(rqst, transactionsList);
    }
    else
    {
        logError("MessageProcessor::transactionsMsg: transactionsList==nullptr");
        return;
    }
    if (transactionsList->size()==0)
    {
        logError("MessageProcessor::transactionsMsg: transactionsList is empty");
        if (!stored) deleteList(transactionsList);
        return;
    }
    if (!stored) deleteList(transactionsList);
}

void MessageProcessor::nextClaimMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::nextClaimMsg");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)message[i]);
    size_t i = 0;
    // extract initial request
    if (i+8 > str.length())
    {
        logError("MessageProcessor::nextClaimMsg: bad format 1");
        return;
    }
    string lenOfRqstStr = str.substr(i, 8);
    i+=8;
    unsigned long long lenOfRqst = u.byteSeqAsUll(lenOfRqstStr);
    if (i+lenOfRqst > str.length())
    {
        logError("MessageProcessor::nextClaimMsg: bad format 2");
        return;
    }
    string rqst = str.substr(i, lenOfRqst);
    i+=lenOfRqst;
    // extract claim entry
    string type14entryStr = str.substr(i, str.length()-i);
    Type14Entry t14e(type14entryStr);
    if (!t14e.isGood())
    {
        logError("MessageProcessor::nextClaimMsg: bad Type14Entry");
        return;
    }
    // sign and send new claim
    if (!t14e.hasPredecessorClaim() || t14e.getPredecessorsCount()>1 || storeProcessedRqst(rqst, nullptr))
    {
        logInfo("MessageProcessor::nextClaimMsg: registering new claim");
        string regRqst;
        RequestBuilder* rqstBuilder = ch->getRqstBuilder();
        CompleteID ownerId = t14e.getOwnerID();
        string key = rqstBuilder->getKeysHandling()->getNameByID(ownerId);
        if (key.length()<=0)
        {
            logError("MessageProcessor::nextClaimMsg: unknown ownerId:");
            logError(ownerId.to27Char().c_str());
            return;
        }
        CryptoPP::RSA::PrivateKey* prvtKey = rqstBuilder->getKeysHandling()->getPrivateKey(key);
        unsigned long notaryNr = ch->getNotaryNr();
        if (rqstBuilder->createClaimRegRqst(&t14e, prvtKey, ownerId, notaryNr, regRqst))
        {
            ch->sendRequest(regRqst);
        }
    }
    else
    {
        logInfo("MessageProcessor::nextClaimMsg: no need to register new claim");
        return;
    }
}

void MessageProcessor::currOrOblInfoMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::currOrOblInfoMsg");
    // create list of type 13 entries as string
    string t13eList;
    for (size_t i=1; i<n; i++) t13eList.push_back((char)message[i]);
    // get type 12 entry
    CIDsSet firstIDs;
    Type12Entry* t12e = extractType12Entry(t13eList, firstIDs, false, false);
    if (t12e==nullptr) return;
    if (!t12e->isGood() || (t12e->underlyingType()!=5 && t12e->underlyingType()!=15))
    {
        delete t12e;
        return;
    }
    Type5Or15Entry *t5Or515e = (Type5Or15Entry*) t12e->underlyingEntry();
    // update liquidities
    LiquiditiesHandling *lh = ch->getRqstBuilder()->getLiquiditiesHandling();
    string name = lh->getName(*(t5Or515e->getByteSeq()));
    if (name.length()>0)
    {
        lh->setId(name, firstIDs.first());
        logInfo("id added to LiquiditiesHandling");
    }
    // clean up
    delete t12e;
}

void MessageProcessor::considerContactInfoRequest(const size_t n, byte *request)
{
    logInfo("new MessageProcessor::considerContactInfoRequest");
    string str;
    for (size_t i=1; i<n; i++) str.push_back((char)request[i]);
    // create ContactInfo
    ContactInfo contactInfo(str);
    ch->tryToStoreContactInfo(contactInfo, true);
}

void MessageProcessor::pblcKeyInfoMsg(const size_t n, byte *message)
{
    logInfo("new MessageProcessor::pblcKeyInfoMsg");
    // create list of type 13 entries as string
    string t13eList;
    for (size_t i=1; i<n; i++) t13eList.push_back((char)message[i]);
    // get type 12 entry
    CIDsSet firstIDs;
    Type12Entry* t12e = extractType12Entry(t13eList, firstIDs, false, false);
    if (t12e==nullptr) return;
    if (!t12e->isGood() || t12e->underlyingType()!=11)
    {
        delete t12e;
        return;
    }
    // update keys
    KeysHandling *keys = ch->getRqstBuilder()->getKeysHandling();
    Type11Entry *t11e = (Type11Entry*) t12e->underlyingEntry();
    string name = keys->getName(*(t11e->getPublicKey()));
    if (name.length()>0)
    {
        keys->setId(name, firstIDs.first());
        keys->updateValidity(name, t11e->getValidityDate());
        logInfo("key id added to keys");
    }
    // clean up
    delete t12e;
}

list<pair<Type12Entry*, CIDsSet*>>* MessageProcessor::extractType12Entries(string &t13eList, const bool keysAlreadyLocked)
{
    list<pair<Type12Entry*, CIDsSet*>>* out = new list<pair<Type12Entry*, CIDsSet*>>();
    size_t i = 0;
    while (i<t13eList.length())
    {
        // get sublist string
        if (i+8 > t13eList.length())
        {
            logError("MessageProcessor::extractType12Entries: bad format 1");
            deleteList(out);
            return nullptr;
        }
        string lenOfSublistStr = t13eList.substr(i, 8);
        i+=8;
        unsigned long long lenOfSublist = u.byteSeqAsUll(lenOfSublistStr);
        if (i+lenOfSublist > t13eList.length())
        {
            logError("MessageProcessor::extractType12Entries: bad format 2");
            deleteList(out);
            return nullptr;
        }
        string sublistSeq = t13eList.substr(i, lenOfSublist);
        i+=lenOfSublist;
        // create sublist
        CIDsSet* firstIDs = new CIDsSet();
        Type12Entry* t12e = extractType12Entry(sublistSeq, *firstIDs, keysAlreadyLocked, false);
        if (t12e == nullptr || firstIDs->size()<=0)
        {
            if (t12e != nullptr) delete t12e;
            delete firstIDs;
            deleteList(out);
            return nullptr;
        }
        // add to out
        out->push_back(pair<Type12Entry*, CIDsSet*>(t12e, firstIDs));
    }
    if (i!=t13eList.length())
    {
        deleteList(out);
        return nullptr;
    }
    return out;
}

Type12Entry* MessageProcessor::extractType12Entry(string &t13eList, CIDsSet &firstIDs, const bool keysAlreadyLocked, const bool firstBlockOnly)
{
    Type12Entry *t12e = nullptr;
    list<Type13Entry*> type13entries;
    size_t i = 0;
    while (i<t13eList.length())
    {
        if (i+8 > t13eList.length())
        {
            logError("MessageProcessor::extractType12Entry: bad format 1");
            deleteList(type13entries);
            if (t12e!=nullptr) delete t12e;
            return nullptr;
        }
        string lenOfT13eStr = t13eList.substr(i, 8);
        i+=8;
        unsigned long long lenOfT13e = u.byteSeqAsUll(lenOfT13eStr);
        if (i+lenOfT13e > t13eList.length())
        {
            logError("MessageProcessor::extractType12Entry: bad format 2");
            deleteList(type13entries);
            if (t12e!=nullptr) delete t12e;
            return nullptr;
        }
        string t13eSeq = t13eList.substr(i, lenOfT13e);
        i+=lenOfT13e;
        Type13Entry *t13e = new Type13Entry(t13eSeq);
        if (!t13e->isGood())
        {
            logError("bad t13e in MessageProcessor::extractType12Entry");
            delete t13e;
            deleteList(type13entries);
            if (t12e!=nullptr) delete t12e;
            return nullptr;
        }
        if (type13entries.size()==0)
        {
            t12e = createT12FromT13Str(t13eSeq);
            if (t12e==nullptr || !t12e->isGood())
            {
                logError("bad t12e in MessageProcessor::extractType12Entry");
                logInfo(t13e->getCompleteID().to27Char().c_str());
                logInfo(t13e->getPredecessorID().to27Char().c_str());
                logInfo(t13e->getFirstID().to27Char().c_str());
                delete t13e;
                deleteList(type13entries);
                return nullptr;
            }
        }
        type13entries.push_back(t13e);
    }
    if (i!=t13eList.length() || t12e==nullptr)
    {
        deleteList(type13entries);
        if (t12e!=nullptr) delete t12e;
        logError("bad t13e list in MessageProcessor::extractType12Entry");
        return nullptr;
    }

    // check signatures
    KeysHandling *keys = ch->getRqstBuilder()->getKeysHandling();
    Type1Entry *type1entry = keys->getType1Entry();
    Type13Entry *predecessorT13e = nullptr;
    Type13Entry *lastFirstT13e = nullptr;
    unsigned long signatureCountInBlock = 0;
    unsigned char status = 3;
    unsigned long long timeLimit = 0;
    bool entryToConfirm = false;
    bool success = false;
    list<Type13Entry*>::iterator it;
    for (it=type13entries.begin(); it!=type13entries.end(); ++it)
    {
        logInfo("MessageProcessor::extractType12Entry: processing type13entry...");
        success = false;
        // conduct basic checks
        Type13Entry *entry = *it;
        CompleteID entryID = entry->getCompleteID();
        if (entryToConfirm && entryID != type1entry->getConfirmationId())
        {
            logError("MessageProcessor::extractType12Entry: missing confirmation entry");
            break;
        }
        if (entryID == type1entry->getConfirmationId()
                && (predecessorT13e == nullptr || status != 2 || signatureCountInBlock==0 || !entryToConfirm))
        {
            logError("MessageProcessor::extractType12Entry: confirmation entry out of line");
            break;
        }
        unsigned long long entryTime = entryID.getTimeStamp();
        unsigned short linCurrent = type1entry->getLineage(entryTime);
        if (linCurrent == 0)
        {
            logError("MessageProcessor::extractType12Entry: bad linCurrent 1");
            break;
        }
        CompleteID predecessorID = entry->getPredecessorID();
        if (predecessorT13e == nullptr && predecessorID != entryID)
        {
            logError("MessageProcessor::extractType12Entry: predecessorT13e == nullptr && predecessorID != entryID");
            break;
        }
        unsigned short linPred = type1entry->getLineage(predecessorID.getTimeStamp());
        if (linPred == 0 || linPred>linCurrent)
        {
            logError("MessageProcessor::extractType12Entry: bad linPred");
            break;
        }
        if (entryID == type1entry->getConfirmationId())
        {
            logInfo("MessageProcessor::extractType12Entry: processing confirmation entry");
            if (signatureCountInBlock==0 || linPred==linCurrent)
            {
                logError("MessageProcessor::extractType12Entry: bad confirmation entry");
                break;
            }
        }
        else if (signatureCountInBlock!=0 && linPred!=linCurrent)
        {
            logError("MessageProcessor::extractType12Entry: bad linCurrent 2");
            break;
        }
        CompleteID firstID = entry->getFirstID();
        CompleteID latestFirstID;
        if (!firstIDs.isEmpty()) latestFirstID = firstIDs.last();
        if (signatureCountInBlock == 0 && firstID != entryID)
        {
            logError("MessageProcessor::extractType12Entry: bad firstID 1");
            break;
        }
        if (signatureCountInBlock != 0 && firstID != latestFirstID)
        {
            logError("MessageProcessor::extractType12Entry: bad firstID 2");
            break;
        }
        if (predecessorT13e != nullptr)
        {
            if (signatureCountInBlock == 0)
            {
                if (latestFirstID != predecessorID || !type1entry->isFresh(predecessorID, entryTime))
                {
                    logError("MessageProcessor::extractType12Entry: bad predecessorID 1");
                    break;
                }
            }
            else
            {
                if (predecessorT13e->getCompleteID() != predecessorID)
                {
                    logError("MessageProcessor::extractType12Entry: bad predecessorID 2");
                    break;
                }
                if (entryID != type1entry->getConfirmationId() && entryTime >= timeLimit)
                {
                    logError("MessageProcessor::extractType12Entry: timeLimit violated");
                    break;
                }
            }
        }

        // check the notary
        unsigned long notaryNr = entry->getNotary();
        if (!type1entry->isActingNotary(notaryNr, entryTime))
        {
            logError("MessageProcessor::extractType12Entry: not acting notary");
            break;
        }

        // get public key of notary
        TNtrNr totalNotaryNr = type1entry->getTotalNotaryNr(notaryNr, entryTime);
        CryptoPP::RSA::PublicKey *publicKey = keys->getNotaryPubKey(totalNotaryNr, keysAlreadyLocked);
        if (publicKey == nullptr)
        {
            logError("MessageProcessor::extractType12Entry: publicKey == nullptr");
            logError(totalNotaryNr.toString().c_str());
            break;
        }

        // check signature
        logInfo("checking signature in MessageProcessor::extractType12Entry");
        string signedSeq(entryID.to20Char());
        signedSeq.append(predecessorID.to20Char());
        signedSeq.append(firstID.to20Char());
        signedSeq.append(*t12e->getByteSeq());
        if (predecessorT13e != nullptr) signedSeq.append(*predecessorT13e->getByteSeq());
        string* signa = entry->getSignature();
        CryptoPP::SecByteBlock signature((const byte*)signa->c_str(), signa->length());
        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA3_384>::Verifier verifier(*publicKey);

        bool correct = verifier.VerifyMessage((const byte*)signedSeq.c_str(),
                                              signedSeq.length(), signature, signature.size());
        if (!correct)
        {
            logError("bad signature in MessageProcessor::extractType12Entry");
            break;
        }

        // update running data
        if (entryID != type1entry->getConfirmationId())
        {
            if (signatureCountInBlock == 0)
            {
                firstIDs.add(entryID);
                lastFirstT13e = entry;
                timeLimit = type1entry->getNotarizationTimeLimit(entryTime);
                logInfo("MessageProcessor::extractType12Entry: new timeLimit: ");
                logInfo(to_string(timeLimit).c_str());
                if (predecessorT13e == nullptr)
                {
                    timeLimit = min(timeLimit, t12e->getNotarizationTimeLimit());
                    logInfo("MessageProcessor::extractType12Entry: timeLimit after min: ");
                    logInfo(to_string(timeLimit).c_str());
                }
            }

            if (entryTime >= timeLimit)
            {
                logError("notarization time limit violated in MessageProcessor::extractType12Entry");
                logError(to_string(entryTime).c_str());
                logError("vs");
                logError(to_string(timeLimit).c_str());
                break;
            }

            signatureCountInBlock++;
            predecessorT13e = entry;

            status = type1entry->notarizationStatus(signatureCountInBlock, entryTime);
            if (status == 2)
            {
                if (linCurrent != type1entry->latestLin())
                {
                    logInfo("MessageProcessor::extractType12Entry: entryToConfirm");
                    entryToConfirm = true;
                }
                else
                {
                    signatureCountInBlock = 0;
                    predecessorT13e = lastFirstT13e;
                }
            }
        }
        else
        {
            logInfo("MessageProcessor::extractType12Entry: confirmation entry");
            signatureCountInBlock = 0;
            predecessorT13e = lastFirstT13e;
            entryToConfirm = false;
        }

        if (!entryToConfirm && status == 2 && !firstIDs.isEmpty()) success=true;
        if (success && firstBlockOnly) break;
    }
    if (entryToConfirm) logError("MessageProcessor::extractType12Entry: remaining entryToConfirm");
    if (status != 2) logError("MessageProcessor::extractType12Entry: remaining status != 2");
    if (firstIDs.isEmpty()) logError("MessageProcessor::extractType12Entry: firstIDs is empty");
    // clean up
    deleteList(type13entries);
    // return result
    if (!success)
    {
        delete t12e;
        firstIDs.clear();
        logError("no success in for loop of MessageProcessor::extractType12Entry");
        return nullptr;
    }
    logInfo("success in for loop of MessageProcessor::extractType12Entry");
    return t12e;
}

Type12Entry* MessageProcessor::createT12FromT13Str(string& str)
{
    const unsigned long long l = str.length();
    if (l < 77 || str.at(0) != 0x2C) return nullptr;
    if (str.substr(1, 20).compare(str.substr(21, 20)) != 0) return nullptr;
    if (str.substr(21, 20).compare(str.substr(41, 20)) != 0) return nullptr;
    string t12eStrWithSigLenStr = str.substr(61, 8);
    string t12eStrLenStr = str.substr(69, 8);
    unsigned long long t12eStrWithSigLen = u.byteSeqAsUll(t12eStrWithSigLenStr);
    unsigned long long t12eStrLen = u.byteSeqAsUll(t12eStrLenStr);
    if (t12eStrWithSigLen <= t12eStrLen+8 || l != t12eStrWithSigLen+69) return nullptr;
    string t12eStr(str.substr(77, t12eStrLen));
    Type12Entry *type12entry = new Type12Entry(t12eStr);
    if (!type12entry->isGood())
    {
        delete type12entry;
        logInfo("bad str in MessageProcessor::createT12FromT13Str(string& str) 1");
        return nullptr;
    }
    return type12entry;
}

void MessageProcessor::deleteList(list<Type13Entry*> type13entries)
{
    list<Type13Entry*>::iterator it;
    for (it=type13entries.begin(); it!=type13entries.end(); ++it)
    {
        delete *it;
    }
    type13entries.clear();
}

void MessageProcessor::setLogger(Logger *l)
{
    log=l;
}

void MessageProcessor::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void MessageProcessor::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}
