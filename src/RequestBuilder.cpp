#include "RequestBuilder.h"

#define maxTimeDiffInS 36000
#define discountTimeBufferInMs 60000

RequestBuilder::RequestBuilder(KeysHandling *k, LiquiditiesHandling *l) :  keys(k), liquidities(l), log(nullptr),
    positiveTimeOffSetInMs(1), negativeTimeOffSetInMs(1)
{
    rng = new CryptoPP::AutoSeededRandomPool();
}

RequestBuilder::~RequestBuilder()
{
    latestClaimsMutex.lock();
    map<pair<string,string>, CIDsSet*>::iterator it1;
    for (it1=latestClaimsId.begin(); it1!=latestClaimsId.end(); ++it1)
    {
        delete it1->second;
    }
    latestClaimsId.clear();

    map<pair<string,string>, Type14Entry*>::iterator it2;
    for (it2=latestClaims.begin(); it2!=latestClaims.end(); ++it2)
    {
        delete it2->second;
    }
    latestClaims.clear();
    latestClaimsMutex.unlock();

    refInfosMutex.lock();
    map<pair<string,string>, RefereeInfo*>::iterator it3;
    for (it3=refInfos.begin(); it3!=refInfos.end(); ++it3)
    {
        delete it3->second;
    }
    refInfos.clear();
    refInfosMutex.unlock();

    notaryInfosMutex.lock();
    map<string, NotaryInfo*>::iterator it4;
    for (it4=notaryInfos.begin(); it4!=notaryInfos.end(); ++it4)
    {
        delete it4->second;
    }
    notaryInfos.clear();
    notaryInfosMutex.unlock();

    delete rng;
}

KeysHandling* RequestBuilder::getKeysHandling()
{
    return keys;
}

LiquiditiesHandling* RequestBuilder::getLiquiditiesHandling()
{
    return liquidities;
}

Type12Entry* RequestBuilder::signEntry(Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &keyID, unsigned long firstNotary)
{
    if (entry==nullptr || !entry->isGood() || entry->getByteSeq()==nullptr)
    {
        logError("bad entry supplied in RequestBuilder::signEntry(Entry* entry)");
        return nullptr;
    }
    if (prvtKey == nullptr)
    {
        logError("bad prvtKey supplied in RequestBuilder::signEntry(Entry* entry)");
        return nullptr;
    }
    unsigned long long currentTimeInMs = currentSyncTimeInMs();
    if (currentTimeInMs == 0)
    {
        logError("bad currentTimeInMs in RequestBuilder::signEntry(Entry* entry)");
        return nullptr;
    }
    logInfo("RequestBuilder::signEntry: building strings");
    Util u;
    string type12entryStr;
    type12entryStr.push_back(0x2B);
    // create string to sign
    string strToSign(*entry->getByteSeq());
    // add length to type12entryStr
    type12entryStr.append(u.UllAsByteSeq(strToSign.length()));
    // continue
    if (keyID.getNotary()>0) strToSign.append(keyID.to20Char());
    strToSign.append(u.UlAsByteSeq(firstNotary));
    strToSign.append(u.UllAsByteSeq(currentTimeInMs + 1000 * 30));
    // sign string
    logInfo("RequestBuilder::signEntry: creating signature");
    CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA3_384>::Signer signer(*prvtKey);
    size_t length = signer.MaxSignatureLength();
    CryptoPP::SecByteBlock signature(length);
    length = signer.SignMessage(*rng, (const byte*) strToSign.c_str(), strToSign.length(), signature);
    signature.resize(length);
    // finish type12entryStr
    logInfo("RequestBuilder::signEntry: creating type12entryStr");
    type12entryStr.append(strToSign);
    type12entryStr.append(u.UllAsByteSeq(signature.size()));
    type12entryStr.append((const char*) signature.BytePtr(), signature.size());
    // create type 12 entry and return
    logInfo("RequestBuilder::signEntry: creating type12entry");
    Type12Entry *type12entry = new Type12Entry(type12entryStr);
    if (!type12entry->isGood())
    {
        logError("RequestBuilder::signEntry: failed to create good type12entry");
        delete type12entry;
        return nullptr;
    }
    return type12entry;
}

Type12Entry* RequestBuilder::packKeyAndSign(string &pubKeyStr, CryptoPP::RSA::PrivateKey* prvtKey, unsigned long firstNotary)
{
    unsigned long long currentTimeInMs = currentSyncTimeInMs();
    if (currentTimeInMs==0) return nullptr;
    unsigned long long validUntil = 1000*60; // minute
    validUntil *= 60; // hour
    validUntil *= 24; // day
    validUntil *= 356; // year
    validUntil *= 60;
    validUntil += currentTimeInMs;
    Type11Entry type11entry(pubKeyStr, validUntil);
    CompleteID dumId;
    return signEntry(&type11entry, prvtKey, dumId, firstNotary);
}

bool RequestBuilder::createPubKeyInfoRqst(string &pubKeyStr, string &rqst)
{
    if (rqst.length()>0 || pubKeyStr.length()<3) return false;
    byte type = 1;
    rqst.push_back(type);
    rqst.append(pubKeyStr);
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createPubKeyRegRqst(string &pubKeyStr, CryptoPP::RSA::PrivateKey* prvtKey, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    Type12Entry* type12entry = packKeyAndSign(pubKeyStr, prvtKey, firstNotary);
    if (type12entry == nullptr) return false;
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createEssentialsRqst(CompleteID &minEntryId, string &rqst)
{
    logInfo("RequestBuilder::createEssentialsRqst");
    if (rqst.length()>0) return false;
    byte type = 10;
    rqst.push_back(type);
    rqst.append(minEntryId.to20Char());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createContactsRqst(string &rqst)
{
    if (rqst.length()>0) return false;
    byte type = 19;
    rqst.push_back(type);
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createHeartBeatRqst(string &rqst)
{
    if (rqst.length()>0) return false;
    byte type = 21;
    rqst.push_back(type);
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createLiquiRegRqst(Type5Or15Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &id, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(id, currentTime)) return false;
    // sign
    logInfo("RequestBuilder::createLiquiRegRqst: signing entry ..");
    Type12Entry* type12entry = signEntry(entry, prvtKey, id, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createLiquiRegRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createClaimRegRqst(Type14Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &id, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(id, currentTime)) return false;
    // sign
    logInfo("RequestBuilder::createClaimRegRqst: signing entry ..");
    Type12Entry* type12entry = signEntry(entry, prvtKey, id, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createClaimRegRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createLiquiInfoRqst(Type5Or15Entry* entry, string &rqst)
{
    if (rqst.length()>0 || entry==nullptr || !entry->isGood()) return false;
    byte type = 2;
    rqst.push_back(type);
    rqst.append(*entry->getByteSeq());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createOfferCancelRqst(string &liqui, double &amount, string &key, CompleteID &offerId, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0 || amount<=0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    pair<string,string> accountPair(key, liqui);
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        return false;
    }
    CompleteID claimIDF = latestClaimsId[accountPair]->first();
    CompleteID claimIDL = latestClaimsId[accountPair]->last();
    Type14Entry* t14eOld = latestClaims[accountPair];
    latestClaimsMutex.unlock();
    CompleteID keyID = keys->getLatestID(key);
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime==0 || !isFresh(keyID, currentTime) || !isFresh(claimIDL, currentTime)
            || !isFresh(offerId, currentTime)) return false;
    // get discount rate
    Type5Or15Entry *t5Or15e = liquidities->getLiqui(liqui);
    if (t5Or15e==nullptr) return false;
    double discountRate = t5Or15e->getRate();
    // consider discount
    Util u;
    unsigned long long discountEndDate = currentTime;
    if (discountRate < 0) discountEndDate+=discountTimeBufferInMs;
    else discountEndDate-=discountTimeBufferInMs;
    double discountFactor = u.getDiscountFactor(claimIDF.getTimeStamp(), discountEndDate, discountRate);
    double totalAmountFromClaim = t14eOld->getTotalAmount() * discountFactor;
    double nonTransferAmountFromClaim = t14eOld->getNonTransferAmount() * discountFactor;
    // create vectors
    vector<CompleteID> predecessors;
    vector<unsigned char> scenarios;
    vector<double> impacts;
    scenarios.push_back(0);
    scenarios.push_back(1);
    predecessors.push_back(claimIDL);
    predecessors.push_back(offerId);
    impacts.push_back(totalAmountFromClaim);
    Type1Entry *t1e = keys->getType1Entry();
    if (t1e == nullptr) return false;
    double feeFactor = 1.0 - t1e->getFee(t1e->getLineage(offerId.getTimeStamp()))/100;
    double amountAfterFee = amount * feeFactor;
    impacts.push_back(amountAfterFee);
    double totalAmount = totalAmountFromClaim + amountAfterFee;
    // build type 14 entry
    Type14Entry t14e(liquiID, keyID, totalAmount, nonTransferAmountFromClaim, impacts, predecessors, scenarios);
    if (!t14e.isGood())
    {
        logError("RequestBuilder::createOfferCancelRqst: bad t14e");
        return false;
    }
    logInfo("RequestBuilder::createOfferCancelRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t14e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createOfferCancelRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createTransferRqst(string &liqui, double &amount, string &key, string &target, unsigned long firstNotary, string &rqst)
{
    CompleteID targetID = keys->getLatestID(target);
    return createTransferRqst(liqui, amount, key, targetID, firstNotary, rqst);
}

bool RequestBuilder::createTransferRqst(string &liqui, double &amount, string &key, CompleteID &targetID, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    pair<string,string> accountPair(key, liqui);
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        return false;
    }
    CompleteID claimID = latestClaimsId[accountPair]->last();
    latestClaimsMutex.unlock();
    CompleteID keyID = keys->getLatestID(key);
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime==0 || !isFresh(keyID, currentTime) || !isFresh(claimID, currentTime)
            || !(targetID.isZero() || isFresh(targetID, currentTime))) return false;
    // build transfer entry
    Type10Entry t10e(liquiID, amount, claimID, targetID, 0);
    if (!t10e.isGood())
    {
        logError("RequestBuilder::createTransferRqst: bad t10e");
        return false;
    }
    logInfo("RequestBuilder::createTransferRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t10e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createTransferRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createOfferAcceptRqst(string &liqui, double &amount, CompleteID &targetID, double &amountR, string &key, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    pair<string,string> accountPair(key, liqui);
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        return false;
    }
    CompleteID claimID = latestClaimsId[accountPair]->last();
    latestClaimsMutex.unlock();
    CompleteID keyID = keys->getLatestID(key);
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime) || !isFresh(claimID, currentTime)
            || !(targetID.isZero() || isFresh(targetID, currentTime))) return false;
    // build transfer entry
    Type10Entry t10e(liquiID, amount, claimID, targetID, amountR);
    if (!t10e.isGood())
    {
        logError("RequestBuilder::createOfferAcceptRqst: bad t10e");
        return false;
    }
    logInfo("RequestBuilder::createOfferAcceptRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t10e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createOfferAcceptRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createExchangeOfferRqst(string &liquiO, double &amountO, string &liquiR, double &amountR, string &key, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0 || amountO<=0 || amountR<=0) return false;
    // get ids
    CompleteID liquiOID = liquidities->getID(liquiO);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    pair<string,string> accountPair(key, liquiO);
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        logError("RequestBuilder::createExchangeOfferRqst: no latestClaimsId");
        return false;
    }
    CompleteID claimID = latestClaimsId[accountPair]->last();
    latestClaimsMutex.unlock();
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiRID = liquidities->getID(liquiR);
    if (liquiRID.getNotary()<=0) return false;
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0) return false;
    if (!isFresh(keyID, currentTime))
    {
        logError("RequestBuilder::createExchangeOfferRqst: keyID not fresh");
        return false;
    }
    if (!isFresh(claimID, currentTime))
    {
        logError("RequestBuilder::createExchangeOfferRqst: claimID not fresh");
        return false;
    }
    // build transfer entry
    Type10Entry t10e(liquiOID, amountO, claimID, liquiRID, amountR);
    if (!t10e.isGood())
    {
        logError("RequestBuilder::createExchangeOfferRqst: bad t10e");
        return false;
    }
    logInfo("RequestBuilder::createExchangeOfferRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t10e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createExchangeOfferRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createTransferRqstRqst(string &liqui, double &amount, string &key, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    CompleteID keyID = keys->getLatestID(key);
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime)) return false;
    // build transfer entry
    Type10Entry t10e(liquiID, 0, keyID, liquiID, amount);
    if (!t10e.isGood())
    {
        logError("RequestBuilder::createTransferRqstRqst: bad t10e");
        return false;
    }
    logInfo("RequestBuilder::createTransferRqstRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t10e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createTransferRqstRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createNotaryApplRequest(string &key, string &description, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get id
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    CompleteID keyID = keys->getLatestID(key);
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime)) return false;
    // build application entry
    Type3Entry t3e(keyID, description);
    if (!t3e.isGood())
    {
        logError("RequestBuilder::createNotaryApplRequest: bad t3e");
        return false;
    }
    logInfo("RequestBuilder::createNotaryApplRequest: signing entry ..");
    // build type 3 entry
    Type12Entry* type12entry = signEntry(&t3e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createNotaryApplRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createRefApplRequest(string &key, string &liqui, unsigned long long &tenureStart, string &description,
        unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    CompleteID keyID = keys->getLatestID(key);
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime)) return false;
    // build application entry
    Type6Entry t6e(keyID, liquiID, tenureStart, description);
    if (!t6e.isGood())
    {
        logError("RequestBuilder::createRefApplRequest: bad t6e");
        return false;
    }
    logInfo("RequestBuilder::createRefApplRequest: signing entry ..");
    // build type 6 entry
    Type12Entry* type12entry = signEntry(&t6e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createRefApplRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createNotaryNoteRequest(string &referee, CompleteID &threadId, string &description, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get notary
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(referee);
    CompleteID keyID = keys->getLatestID(referee);
    string *pubKeyStr = keys->getPublicKeyStr(referee);
    TNtrNr tNtrNr = keys->getNotary(pubKeyStr);
    if (tNtrNr.getNotaryNr()<=0)
    {
        logError("RequestBuilder::createNotaryNoteRequest: bad notary");
        return false;
    }
    // check id for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(threadId, currentTime) || !isFresh(keyID, currentTime)) return false;
    // build Type4Entry entry
    Type4Entry t4e(tNtrNr.getNotaryNr(), threadId, description);
    if (!t4e.isGood())
    {
        logError("RequestBuilder::createNotaryNoteRequest: bad t4e");
        return false;
    }
    logInfo("RequestBuilder::createNotaryNoteRequest: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t4e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createNotaryNoteRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

unsigned long RequestBuilder::createNotaryRegistrationRequest(string &key, CompleteID &terminationId, unsigned long firstNotary, string &rqst)
{
    // get keys and predecessor id
    CompleteID keyID = keys->getLatestID(key);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    string *publicKeyStr = keys->getPublicKeyStr(key);
    CompleteID predId = keys->getLatestT2eId();
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(terminationId, currentTime) || !isFresh(keyID, currentTime)
            || (!predId.isZero() && !isFresh(predId, currentTime))) return 0;
    // get future notary num
    Type1Entry *t1e = keys->getType1Entry();
    if (t1e == nullptr) return 0;
    unsigned short lineage = t1e->latestLin();
    unsigned long num = keys->getAppointedNotariesNum(lineage);
    if ((num>0 && predId.getNotary()<=0) || (num<=0 && !predId.isZero())) return 0;
    num += t1e->getMaxActing(lineage);
    num += 1;
    // build Type2Entry entry
    Type2Entry t2e(predId, terminationId, *publicKeyStr);
    if (!t2e.isGood())
    {
        logError("RequestBuilder::createNotaryRegistrationRequest: bad t2e");
        return 0;
    }
    logInfo("RequestBuilder::createNotaryRegistrationRequest: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t2e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return 0;
    logInfo("RequestBuilder::createNotaryRegistrationRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return 0;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return num;
}

bool RequestBuilder::createRefNoteRequest(string &referee, string &liqui, CompleteID &threadId, string &description,
        unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(referee);
    CompleteID keyID = keys->getLatestID(referee);
    pair<string,string> accountPair(referee, liqui);
    // claimID
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        return false;
    }
    CompleteID claimID = latestClaimsId[accountPair]->last();
    latestClaimsMutex.unlock();
    // appointmentID
    refInfosMutex.lock();
    if (refInfos.count(accountPair)!=1)
    {
        refInfosMutex.unlock();
        return false;
    }
    CompleteID appointmentID = refInfos[accountPair]->getAppointmentId();
    refInfosMutex.unlock();
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime) || !isFresh(claimID, currentTime)
            || !isFresh(threadId, currentTime)) return false;
    if (liquidities->getNameById(appointmentID).length()<=0
            && !isFresh(appointmentID, currentTime)) return false;
    // build Type8Entry entry
    Type8Entry t8e(appointmentID, threadId, claimID, description);
    if (!t8e.isGood())
    {
        logError("RequestBuilder::createRefNoteRequest: bad t8e");
        return false;
    }
    logInfo("RequestBuilder::createRefNoteRequest: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t8e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createRefNoteRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createOpPrRequest(string &key, string &liqui, double &amount, double &fee, double &forfeit,
                                       unsigned long &processingTime, string &description, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0) return false;
    // get ids
    CompleteID liquiID = liquidities->getID(liqui);
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    CompleteID keyID = keys->getLatestID(key);
    pair<string,string> accountPair(key, liqui);
    latestClaimsMutex.lock();
    if (latestClaimsId.count(accountPair)!=1)
    {
        latestClaimsMutex.unlock();
        return false;
    }
    CompleteID claimID = latestClaimsId[accountPair]->last();
    latestClaimsMutex.unlock();
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime) || !isFresh(claimID, currentTime)) return false;
    // build operation proposal entry
    Type7Entry t7e(keyID, liquiID, amount, fee, forfeit, claimID, processingTime, description);
    if (!t7e.isGood())
    {
        logError("RequestBuilder::createOpPrRequest: bad t7e");
        return false;
    }
    logInfo("RequestBuilder::createOpPrRequest: signing entry ..");
    // build type 7 entry
    Type12Entry* type12entry = signEntry(&t7e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createOpPrRequest: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::createPrintAndTransferRqst(string &liqui, double &amount, string &key, string &target, unsigned long firstNotary, string &rqst)
{
    CompleteID targetID = keys->getLatestID(target);
    return createPrintAndTransferRqst(liqui, amount, key, targetID, firstNotary, rqst);
}

bool RequestBuilder::createPrintAndTransferRqst(string &obl, double &amount, string &key, CompleteID &targetID, unsigned long firstNotary, string &rqst)
{
    if (rqst.length()>0 || amount<=0) return false;
    if (targetID.getNotary()<=0)
    {
        logError("RequestBuilder::createPrintAndTransferRqst: bad targetID");
        return false;
    }
    // get ids
    CompleteID oblID = liquidities->getID(obl);
    if (oblID.getNotary()<=0)
    {
        logError("RequestBuilder::createPrintAndTransferRqst: bad oblID");
        return false;
    }
    CryptoPP::RSA::PrivateKey* prvtKey = keys->getPrivateKey(key);
    CompleteID keyID = keys->getLatestID(key);
    if (keyID.getNotary()<=0)
    {
        logError("RequestBuilder::createPrintAndTransferRqst: bad keyID");
        return false;
    }
    // check ids for freshness
    unsigned long long currentTime = currentSyncTimeInMs();
    if (currentTime == 0 || !isFresh(keyID, currentTime) || !isFresh(targetID, currentTime)) return false;
    // build transfer entry
    Type10Entry t10e(oblID, amount, keyID, targetID, 0);
    if (!t10e.isGood())
    {
        logError("RequestBuilder::createPrintAndTransferRqst: bad t10e");
        return false;
    }
    logInfo("RequestBuilder::createPrintAndTransferRqst: signing entry ..");
    // build type 12 entry
    Type12Entry* type12entry = signEntry(&t10e, prvtKey, keyID, firstNotary);
    if (type12entry == nullptr) return false;
    logInfo("RequestBuilder::createPrintAndTransferRqst: type12entry created ..");
    string *type12entryStr = type12entry->getByteSeq();
    if (type12entryStr == nullptr)
    {
        delete type12entry;
        return false;
    }
    // pack and return
    byte type = 0;
    rqst.push_back(type);
    rqst.append(*type12entryStr);
    packRequest(&rqst);
    delete type12entry;
    return true;
}

bool RequestBuilder::isFresh(CompleteID &id, unsigned long long currentTime)
{
    string keyName = keys->getNameByID(id);
    if (keyName.length()>0 && keys->getInitialID(keyName).getNotary()>0) return true; // fresh by definition

    string liquiName = liquidities->getNameById(id);
    if (liquiName.length()>0 && liquidities->getID(liquiName).getNotary()>0) return true; // fresh by definition

    Type1Entry *t1e = keys->getType1Entry();
    if (t1e == nullptr) return false;
    return t1e->isFresh(id, currentTime);
}

bool RequestBuilder::createIdInfoRqst(CompleteID id, string &rqst)
{
    if (rqst.length()>0) return false;
    byte type = 3;
    rqst.push_back(type);
    rqst.append(id.to20Char());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createRefInfoRqst(string &key, string &liqui, string &rqst)
{
    if (rqst.length()>0) return false;
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiID = liquidities->getID(liqui);
    if (keyID.getNotary()<=0 || liquiID.getNotary()<=0) return false;
    byte type = 8;
    rqst.push_back(type);
    rqst.append(keyID.to20Char());
    rqst.append(liquiID.to20Char());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createNotaryInfoRqst(string *publicKeyStr, string &rqst)
{
    if (rqst.length()>0 || publicKeyStr==nullptr) return false;
    if (publicKeyStr->length()<=0) return false;
    byte type = 11;
    rqst.push_back(type);
    Util u;
    rqst.append(u.UlAsByteSeq(publicKeyStr->length()));
    rqst.append(*publicKeyStr);
    CompleteID zeroId;
    rqst.append(zeroId.to20Char());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createNotaryInfoRqst(string *publicKeyStr, string &liqui, string &rqst)
{
    if (rqst.length()>0 || publicKeyStr==nullptr) return false;
    CompleteID liquiID = liquidities->getID(liqui);
    if (publicKeyStr->length()<=0 || liquiID.getNotary()<=0) return false;
    byte type = 11;
    rqst.push_back(type);
    Util u;
    rqst.append(u.UlAsByteSeq(publicKeyStr->length()));
    rqst.append(*publicKeyStr);
    rqst.append(liquiID.to20Char());
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createClaimsInfoRqst(string &key, string &liqui, CompleteID &maxClaimId, unsigned short maxNum, string &rqst)
{
    if (rqst.length()>0 || maxNum<1) return false;
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiID = liquidities->getID(liqui);
    if (keyID.getNotary()<=0 || liquiID.getNotary()<=0) return false;
    byte type = 4;
    rqst.push_back(type);
    rqst.append(keyID.to20Char());
    rqst.append(liquiID.to20Char());
    rqst.append(maxClaimId.to20Char());
    Util u;
    rqst.append(u.UsAsByteSeq(maxNum));
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createDecThrInfoRqst(unsigned char &spec, string &key, CompleteID &minApplId, unsigned short maxNum, string &rqst)
{
    if (rqst.length()>0 || maxNum<1) return false;
    CompleteID keyID(0,0,0);
    if (key.length()!=0)
    {
        keyID = keys->getLatestID(key);
        if (keyID.getNotary()<=0) return false;
    }
    byte type = 7;
    rqst.push_back(type);
    Util u;
    rqst.append(u.UcAsByteSeq(spec));
    rqst.append(keyID.to20Char());
    CompleteID zeroId;
    rqst.append(zeroId.to20Char());
    rqst.append(minApplId.to20Char());
    rqst.append(u.UsAsByteSeq(maxNum));
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createDecThrInfoRqst(unsigned char &spec, string &key, string &liqui, CompleteID &minApplId, unsigned short maxNum, string &rqst)
{
    if (rqst.length()>0 || maxNum<1) return false;
    CompleteID keyID(0,0,0);
    if (key.length()!=0)
    {
        keyID = keys->getLatestID(key);
        if (keyID.getNotary()<=0) return false;
    }
    CompleteID liquiID = liquidities->getID(liqui);
    if (liquiID.getNotary()<=0) return false;
    byte type = 7;
    rqst.push_back(type);
    Util u;
    rqst.append(u.UcAsByteSeq(spec));
    rqst.append(keyID.to20Char());
    rqst.append(liquiID.to20Char());
    rqst.append(minApplId.to20Char());
    rqst.append(u.UsAsByteSeq(maxNum));
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createTransRqstsInfoRqst(string &key, string &liqui, CompleteID &maxTransId, unsigned short maxNum, string &rqst)
{
    if (rqst.length()>0 || maxNum<1) return false;
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiID = liquidities->getID(liqui);
    if (keyID.getNotary()<=0 || liquiID.getNotary()<=0) return false;
    byte type = 6;
    rqst.push_back(type);
    rqst.append(keyID.to20Char());
    rqst.append(liquiID.to20Char());
    rqst.append(maxTransId.to20Char());
    Util u;
    rqst.append(u.UsAsByteSeq(maxNum));
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createExchangeOffersInfoRqst(string &key, string &liquiO, unsigned short &rangeNum, string &liquiR, unsigned short maxNum, string &rqst)
{
    if (rqst.length()>0 || maxNum<1) return false;
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiOID = liquidities->getID(liquiO);
    CompleteID liquiRID = liquidities->getID(liquiR);
    if (liquiOID.getNotary()<=0 || liquiRID.getNotary()<=0) return false;
    byte type = 9;
    rqst.push_back(type);
    rqst.append(keyID.to20Char());
    rqst.append(liquiOID.to20Char());
    Util u;
    rqst.append(u.UsAsByteSeq(rangeNum));
    rqst.append(liquiRID.to20Char());
    rqst.append(u.UsAsByteSeq(maxNum));
    packRequest(&rqst);
    return true;
}

bool RequestBuilder::createNextClaimRqst(string &key, string &liqui, string &rqst)
{
    if (rqst.length()>0) return false;
    CompleteID keyID = keys->getLatestID(key);
    CompleteID liquiID = liquidities->getID(liqui);
    if (keyID.getNotary()<=0 || liquiID.getNotary()<=0) return false;
    byte type = 5;
    rqst.push_back(type);
    rqst.append(keyID.to20Char());
    rqst.append(liquiID.to20Char());
    packRequest(&rqst);
    return true;
}

RefereeInfo* RequestBuilder::getRefInfo(string &key, string &liqui)
{
    refInfosMutex.lock();
    pair<string,string> accountPair(key, liqui);
    // get RefereeInfo
    if (refInfos.count(accountPair)<1)
    {
        refInfosMutex.unlock();
        return nullptr;
    }
    RefereeInfo* out = refInfos[accountPair];
    refInfosMutex.unlock();
    return out;
}

NotaryInfo* RequestBuilder::getNotaryInfo(string &key)
{
    notaryInfosMutex.lock();
    // get NotaryInfo
    if (notaryInfos.count(key)<1)
    {
        notaryInfosMutex.unlock();
        return nullptr;
    }
    NotaryInfo* out = notaryInfos[key];
    notaryInfosMutex.unlock();
    return out;
}

bool RequestBuilder::setRefInfo(string &key, string &liqui, RefereeInfo* refInfo)
{
    logInfo("RequestBuilder::setRefInfo");
    if (refInfo==nullptr) return false;
    refInfosMutex.lock();
    pair<string,string> accountPair(key, liqui);
    // update refInfos
    if (refInfos.count(accountPair)<1)
    {
        refInfos.insert(pair<pair<string,string>, RefereeInfo*>(accountPair, refInfo));
    }
    else
    {
        if (refInfos[accountPair] != nullptr) delete refInfos[accountPair];
        refInfos[accountPair]=refInfo;
    }
    refInfosMutex.unlock();
    return true;
}

bool RequestBuilder::setNotaryInfo(string &key, NotaryInfo* notaryInfo)
{
    logInfo("RequestBuilder::setNotaryInfo");
    if (notaryInfo==nullptr) return false;
    notaryInfosMutex.lock();
    // update NotaryInfo
    if (notaryInfos.count(key)<1)
    {
        notaryInfos.insert(pair<string, NotaryInfo*>(key, notaryInfo));
    }
    else
    {
        if (notaryInfos[key] != nullptr) delete notaryInfos[key];
        notaryInfos[key]=notaryInfo;
    }
    notaryInfosMutex.unlock();
    return true;
}

bool RequestBuilder::setLatestClaim(string &key, string &liqui, Type14Entry* t14e, CIDsSet* ids)
{
    latestClaimsMutex.lock();
    pair<string,string> accountPair(key, liqui);
    // update id
    if (latestClaimsId.count(accountPair)<1)
    {
        latestClaimsId.insert(pair<pair<string,string>, CIDsSet*>(accountPair, ids));
    }
    else if (latestClaimsId[accountPair]->last() > ids->last())
    {
        logError("RequestBuilder::setLatestClaimID: trying to downgrade LatestClaimID");
        latestClaimsMutex.unlock();
        return false;
    }
    else
    {
        latestClaimsId[accountPair]->clear();
        delete latestClaimsId[accountPair];
        latestClaimsId[accountPair]=ids;
    }
    // update Type14Entry
    if (latestClaims.count(accountPair)<1)
    {
        latestClaims.insert(pair<pair<string,string>, Type14Entry*>(accountPair, t14e));
    }
    else
    {
        if (latestClaims[accountPair] != nullptr) delete latestClaims[accountPair];
        latestClaims[accountPair]=t14e;
    }
    latestClaimsMutex.unlock();
    return true;
}

Type14Entry* RequestBuilder::getLatestT14Ecopy(string &key, string &liqui)
{
    pair<string,string> accountPair(key, liqui);
    latestClaimsMutex.lock();
    if (latestClaims.count(accountPair)<1)
    {
        latestClaimsMutex.unlock();
        return nullptr;
    }
    Type14Entry* out = (Type14Entry*) latestClaims[accountPair]->createCopy();
    latestClaimsMutex.unlock();
    return out;
}

void RequestBuilder::packRequest(string *rqst)
{
    unsigned long checkSum = 0;
    for (unsigned long i=0; i<rqst->length(); i++)
    {
        checkSum = addModulo(checkSum, (byte) rqst->at(i));
    }
    Util u;
    string lenAsSeq = u.UlAsByteSeq(rqst->length());
    string lenAsSeqReverse;
    lenAsSeqReverse.push_back(lenAsSeq.at(3));
    lenAsSeqReverse.push_back(lenAsSeq.at(2));
    lenAsSeqReverse.push_back(lenAsSeq.at(1));
    lenAsSeqReverse.push_back(lenAsSeq.at(0));
    rqst->insert(0, lenAsSeqReverse);
    string checkSumAsSeq = u.UlAsByteSeq(checkSum);
    rqst->push_back(checkSumAsSeq.at(3));
    rqst->push_back(checkSumAsSeq.at(2));
    rqst->push_back(checkSumAsSeq.at(1));
    rqst->push_back(checkSumAsSeq.at(0));
}

unsigned long RequestBuilder::addModulo(unsigned long a, unsigned long b)
{
    const unsigned long diff = maxLong - a;
    if (b>diff) return b-diff-1;
    else return a+b;
}

unsigned long long RequestBuilder::currentSyncTimeInMs()
{
    timeOffsetMutex.lock();
    if (positiveTimeOffSetInMs>0 && negativeTimeOffSetInMs>0)
    {
        timeOffsetMutex.unlock();
        return 0;
    }
    using namespace std::chrono;
    milliseconds ms = duration_cast< milliseconds >(
                          system_clock::now().time_since_epoch()
                      );
    unsigned long long out = ms.count();
    out += positiveTimeOffSetInMs;
    out -= negativeTimeOffSetInMs;
    timeOffsetMutex.unlock();
    return out;
}

void RequestBuilder::setCurrentTimeInMs(unsigned long long timeInMs)
{
    // get local time
    using namespace std::chrono;
    milliseconds ms = duration_cast< milliseconds >(
                          system_clock::now().time_since_epoch()
                      );
    unsigned long long localTimeInMs = ms.count();
    // test plausibility
    unsigned long long maxTimeDiffInMs = maxTimeDiffInS;
    maxTimeDiffInMs*=1000;
    if (localTimeInMs > timeInMs + maxTimeDiffInMs || timeInMs > localTimeInMs + maxTimeDiffInMs)
    {
        timeOffsetMutex.lock();
        positiveTimeOffSetInMs=1;
        negativeTimeOffSetInMs=1;
        timeOffsetMutex.unlock();
        logError("RequestBuilder::setCurrentTimeInMs: incompatible time");
        logError(to_string(timeInMs).c_str());
        logError(to_string(localTimeInMs).c_str());
        return;
    }
    // store offset
    timeOffsetMutex.lock();
    if (timeInMs>localTimeInMs)
    {
        positiveTimeOffSetInMs=timeInMs-localTimeInMs;
        negativeTimeOffSetInMs=0;
    }
    else
    {
        negativeTimeOffSetInMs=localTimeInMs-timeInMs;
        positiveTimeOffSetInMs=0;
    }
    timeOffsetMutex.unlock();
}

void RequestBuilder::setLogger(Logger *l)
{
    log=l;
}

void RequestBuilder::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void RequestBuilder::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}
