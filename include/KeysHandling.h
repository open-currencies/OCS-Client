#ifndef KEYSHANDLING_H
#define KEYSHANDLING_H

#include <string>
#include "rsa.h"
#include "secblock.h"
#include "pssr.h"
#include "sha3.h"
#include "osrng.h"
#include "CompleteID.h"
#include "TNtrNr.h"
#include <map>
#include <set>
#include <mutex>
#include "nlohmann/json.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include "files.h"
#include "Type1Entry.h"
#include "CIDsSet.h"

using namespace std;
using json = nlohmann::json;

class KeysHandling
{
public:
    KeysHandling();
    ~KeysHandling();
    CompleteID getLatestID(string &name);
    CompleteID getInitialID(string &name);
    CryptoPP::RSA::PrivateKey* getPrivateKey(string &name);
    string* getPublicKeyStr(string &name);
    string getLocalPrvtKeyUnreg();
    string getLocalPblcKeyUnreg();
    string getName(string &publicKeyStr);
    string getNameByID(CompleteID &id);
    void loadPrivateKeysNames(list<string> &namesList);
    void loadPublicKeysNames(list<string> &namesList);
    bool addNameAndId(string &name, CompleteID &id);
    bool setId(string name, CompleteID id);
    bool addIdAlias(string name, CompleteID id);
    bool updateValidity(string name, unsigned long long validUntil);
    bool addNotaryKeyEtc(string &publicKeyStr, CompleteID type2entryId, CompleteID terminationId);
    TNtrNr getNotary(string *publicKeyStr);
    void loadPrivateKeys();
    void savePublicKeysIDs();
    Type1Entry* getType1Entry();
    CryptoPP::RSA::PublicKey* getNotaryPubKey(TNtrNr totalNotaryNr, const bool wasAlreadyLocked);
    string* getNotaryPubKeyStr(TNtrNr totalNotaryNr);
    CompleteID getLatestT2eId();
    void setLatestT2eId(CompleteID id);
    CompleteID getLastT2eIdFirst();
    CompleteID getNotApplTerminationId(TNtrNr totalNotaryNr);
    CompleteID getType2EntryIdFirst(TNtrNr totalNotaryNr);
    unsigned long getAppointedNotariesNum(unsigned short lineage);
    void loadType1Entry();
    void tryToStorePublicKey(string &name, string str);
    void lock();
    void unlock();
protected:
private:
    mutex keys_mutex;

    // type 1 entry
    Type1Entry* type1entry;

    // notaries public keys
    map<TNtrNr, CryptoPP::RSA::PublicKey*, TNtrNr::CompareTNtrNrs> notariesKeys;
    map<TNtrNr, string, TNtrNr::CompareTNtrNrs> notariesKeysStr;
    map<TNtrNr, CompleteID, TNtrNr::CompareTNtrNrs> type2entryIdsFirst;
    map<TNtrNr, CompleteID, TNtrNr::CompareTNtrNrs> terminationIds;
    map<unsigned short, unsigned long> appointedNotariesNum;
    map<string, TNtrNr> notaryNumByPublicKey;
    CompleteID latestT2eId;
    // private keys
    map<string, CryptoPP::RSA::PrivateKey*> localPrivateKeys;
    list<string> localPrvtKeysUnreg; // the values are the key names
    // public keys
    map<string, CompleteID> localPublicKeysIDs; // the key here is the name and the value the CID
    map<string, string> localPublicKeys; // the key here is the name and the value the public key
    map<string, string> localPublicKeysNames; // the key here is the public key and the value the name
    map<CompleteID, string, CompleteID::CompareIDs> localPublicKeysNamesByID; // the key here is the id and the value the name
    list<string> localPblcKeysUnreg; // the values are the key names

    // some secondary information for public keys
    map<string, CIDsSet*> keyIdAliases;
    map<CompleteID, string, CompleteID::CompareIDs> nameByAliasId; // the key here is the id and the value the name
    map<string, unsigned long long> validity;

    static bool toString(CryptoPP::RSA::PublicKey &publicKey, string &str);
    static bool has_suffix(const std::string &str, const std::string &suffix);

    void loadPublicKeys();
};

#endif // KEYSHANDLING_H
