#ifndef REQUESTBUILDER_H
#define REQUESTBUILDER_H

#include "KeysHandling.h"
#include "LiquiditiesHandling.h"
#include "Entry.h"
#include "Type10Entry.h"
#include "Type11Entry.h"
#include "Type12Entry.h"
#include "Type14Entry.h"
#include "TNtrNr.h"
#include "Util.h"
#include "Logger.h"
#include <mutex>
#include "RefereeInfo.h"
#include "NotaryInfo.h"
#include <FL/fl_ask.H>

#define maxLong 4294967295
typedef unsigned char byte;

class RequestBuilder
{
public:
    RequestBuilder(KeysHandling *k, LiquiditiesHandling *l);
    ~RequestBuilder();
    void setLogger(Logger *l);
    KeysHandling* getKeysHandling();
    LiquiditiesHandling* getLiquiditiesHandling();
    bool createHeartBeatRqst(string &rqst);
    bool createContactsRqst(string &rqst);
    bool createPubKeyRegRqst(string &pubKeyStr, CryptoPP::RSA::PrivateKey* prvtKey, unsigned long firstNotary, string &rqst);
    bool createPubKeyInfoRqst(string &pubKeyStr, string &rqst);
    bool createLiquiRegRqst(Type5Or15Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &id, unsigned long firstNotary, string &rqst);
    bool createClaimRegRqst(Type14Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &id, unsigned long firstNotary, string &rqst);
    bool createOfferCancelRqst(string &liqui, double &amount, string &key, CompleteID &offerId, unsigned long firstNotary, string &rqst);
    bool createLiquiInfoRqst(Type5Or15Entry* entry, string &rqst);
    bool createTransferRqst(string &liqui, double &amount, string &key, string &target, unsigned long firstNotary, string &rqst);
    bool createTransferRqst(string &liqui, double &amount, string &key, CompleteID &targetID, unsigned long firstNotary, string &rqst);
    bool createExchangeOfferRqst(string &liquiO, double &amountO, string &liquiR, double &amountR, string &key, unsigned long firstNotary, string &rqst);
    bool createOfferAcceptRqst(string &liqui, double &amount, CompleteID &targetID, double &amountR, string &key, unsigned long firstNotary, string &rqst);
    bool createPrintAndTransferRqst(string &obl, double &amount, string &key, string &target, unsigned long firstNotary, string &rqst);
    bool createPrintAndTransferRqst(string &obl, double &amount, string &key, CompleteID &targetID, unsigned long firstNotary, string &rqst);
    bool createTransferRqstRqst(string &liqui, double &amount, string &key, unsigned long firstNotary, string &rqst);
    bool createClaimsInfoRqst(string &key, string &liqui, CompleteID &maxClaimId, unsigned short maxNum, string &rqst);
    bool createRefInfoRqst(string &key, string &liqui, string &rqst);
    bool createNotaryInfoRqst(string *publicKeyStr, string &liqui, string &rqst);
    bool createNotaryInfoRqst(string *publicKeyStr, string &rqst);
    bool createTransRqstsInfoRqst(string &key, string &liqui, CompleteID &maxTransId, unsigned short maxNum, string &rqst);
    bool createExchangeOffersInfoRqst(string &key, string &liquiO, unsigned short &rangeNum, string &liquiR, unsigned short maxNum, string &rqst);
    bool createNextClaimRqst(string &key, string &liqui, string &rqst);
    bool setLatestClaim(string &key, string &liqui, Type14Entry* t14e, CIDsSet* ids);
    bool setRefInfo(string &key, string &liqui, RefereeInfo* refInfo);
    bool setNotaryInfo(string &key, NotaryInfo* notaryInfo);
    RefereeInfo* getRefInfo(string &key, string &liqui);
    NotaryInfo* getNotaryInfo(string &key);
    bool createIdInfoRqst(CompleteID id, string &rqst);
    bool createOpPrRequest(string &key, string &liqui, double &amount, double &fee, double &forfeit,
                           unsigned long &processingTime, string &description, unsigned long firstNotary, string &rqst);
    bool createRefApplRequest(string &key, string &liqui, unsigned long long &tenureStart, string &description,
                              unsigned long firstNotary, string &rqst);
    bool createDecThrInfoRqst(unsigned char &spec, string &key, string &liqui, CompleteID &minApplId, unsigned short maxNum, string &rqst);
    bool createDecThrInfoRqst(unsigned char &spec, string &key, CompleteID &minApplId, unsigned short maxNum, string &rqst);
    Type14Entry* getLatestT14Ecopy(string &key, string &liqui);
    bool createRefNoteRequest(string &referee, string &liqui, CompleteID &threadId, string &description,
                              unsigned long firstNotary, string &rqst);
    bool createNotaryNoteRequest(string &referee, CompleteID &threadId, string &description, unsigned long firstNotary, string &rqst);
    bool createEssentialsRqst(CompleteID &minEntryId, string &rqst);
    bool createNotaryApplRequest(string &key, string &description, unsigned long firstNotary, string &rqst);
    unsigned long createNotaryRegistrationRequest(string &key, CompleteID &terminationId, unsigned long firstNotary, string &rqst);
    unsigned long long currentSyncTimeInMs();
    void setCurrentTimeInMs(unsigned long long timeInMs);
    void packRequest(string *rqst);
protected:
private:
    KeysHandling *keys;
    LiquiditiesHandling *liquidities;
    Logger *log;
    Type12Entry* signEntry(Entry* entry, CryptoPP::RSA::PrivateKey* prvtKey, CompleteID &keyID, unsigned long firstNotary);
    Type12Entry* packKeyAndSign(string &pubKeyStr, CryptoPP::RSA::PrivateKey* prvtKey, unsigned long firstNotary);
    unsigned long addModulo(unsigned long a, unsigned long b);
    void logInfo(const char *msg);
    void logError(const char *msg);
    bool isFresh(CompleteID &id, unsigned long long currentTime);

    CryptoPP::AutoSeededRandomPool *rng;

    mutex timeOffsetMutex;
    unsigned long long positiveTimeOffSetInMs;
    unsigned long long negativeTimeOffSetInMs;

    mutex latestClaimsMutex;
    map<pair<string,string>, CIDsSet*> latestClaimsId; // the key here is a pair of strings (keyName+liquiName)
    map<pair<string,string>, Type14Entry*> latestClaims;

    mutex refInfosMutex;
    map<pair<string,string>, RefereeInfo*> refInfos;

    mutex notaryInfosMutex;
    map<string, NotaryInfo*> notaryInfos;
};

#endif // REQUESTBUILDER_H
