#include "KeysHandling.h"

// NOTE: for mingw: replace "stat" with "_stat" for struct and function

KeysHandling::KeysHandling() : type1entry(nullptr)
{
    // try to load from json file
    string fileNameJ("conf/keysIDs.json");
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

    // read data from keysIDs
    for (json::iterator it = j.begin(); it != j.end(); ++it)
    {
        string name = it.key();
        if (name.length()<1 || localPublicKeysIDs.count(name)!=0) continue;
        json ids = it.value();
        unsigned long c = 0;
        for (json::iterator it2 = ids.begin(); it2 != ids.end(); ++it2)
        {
            string idStr = *it2;
            CompleteID id(idStr);
            if ((idStr.length()!=40 && idStr.length()!=27) || id.getNotary()<=0) continue;
            if (c==0) addNameAndId(name, id);
            else addIdAlias(name, id);
            c++;
        }
    }

    // load public keys from dir
    loadPublicKeys();

    // load private keys from dir
    loadPrivateKeys();

    // load type1entry
    loadType1Entry();
}

void KeysHandling::loadType1Entry()
{
    keys_mutex.lock();

    if (type1entry!=nullptr)
    {
        keys_mutex.unlock();
        return;
    }
    else
    {
        type1entry=new Type1Entry("conf/type1entry");
        if (!((Type1Entry*)type1entry)->isGood())
        {
            delete type1entry;
            type1entry = nullptr;
            keys_mutex.unlock();
            return;
        }
    }

    // initialize notaries mentioned in the type1entry
    unsigned short latestLin = ((Type1Entry*)type1entry)->latestLin();
    for (unsigned short i=1; i<=latestLin; i++)
    {
        unsigned long N = ((Type1Entry*)type1entry)->getMaxActing(i);
        for (unsigned long j=1; j<=N; j++)
        {
            TNtrNr tNtrNr(i,j);
            string *publicKeyStr = ((Type1Entry*)type1entry)->getPublicKey(i,j);
            CryptoPP::RSA::PublicKey *publicKey = new CryptoPP::RSA::PublicKey();
            CryptoPP::ByteQueue bq;
            bq.Put((const byte*)publicKeyStr->c_str(), publicKeyStr->length());
            publicKey->Load(bq);
            notariesKeys.insert(pair<TNtrNr, CryptoPP::RSA::PublicKey*>(tNtrNr, publicKey));
            notariesKeysStr.insert(pair<TNtrNr, string>(tNtrNr, *publicKeyStr));
            notaryNumByPublicKey.insert(pair<string, TNtrNr>(*publicKeyStr, tNtrNr));
        }
        appointedNotariesNum.insert(pair<unsigned short, unsigned long>(i, 0));
    }

    keys_mutex.unlock();
}

void KeysHandling::loadPrivateKeys()
{
    keys_mutex.lock();
    DIR *dir;
    struct dirent *ent;
    string keysDir("conf/keys/");
    if ((dir = opendir(keysDir.c_str())) != NULL)
    {
        string suffix = ".PRVTkey";
        CryptoPP::AutoSeededRandomPool rng;
        while ((ent = readdir(dir)) != NULL)
        {
            string filename = ent->d_name;
            if (!has_suffix(filename, suffix)) continue;
            string name = filename.substr(0, filename.size() - suffix.size());
            if (name.length()<1 || localPrivateKeys.count(name)!=0) continue;
            string pathAndName;
            pathAndName.append(keysDir);
            pathAndName.append(filename);
            CryptoPP::RSA::PrivateKey *privateKey = new CryptoPP::RSA::PrivateKey();
            CryptoPP::ByteQueue byteQueue;
            CryptoPP::FileSource file(pathAndName.c_str(), true);
            file.TransferTo(byteQueue);
            byteQueue.MessageEnd();
            privateKey->Load(byteQueue);
            if(privateKey->Validate(rng, 3))
            {
                // store private key
                localPrivateKeys.insert(pair<string, CryptoPP::RSA::PrivateKey*>(name, privateKey));
                if (localPublicKeysIDs.count(name)!=1)
                {
                    localPrvtKeysUnreg.push_back(name);
                }
                // store public key
                CryptoPP::RSA::PublicKey publicKey(*privateKey);
                string str;
                if (toString(publicKey, str) && str.size() > 2)
                {
                    localPublicKeys.insert(pair<string, string>(name, str));
                    localPublicKeysNames.insert(pair<string, string>(str, name));
                    if (localPublicKeysIDs.count(name)!=1)
                    {
                        localPblcKeysUnreg.push_back(name);
                    }
                }
            }
        }
        closedir(dir);
    }
    keys_mutex.unlock();
}

void KeysHandling::loadPublicKeys()
{
    keys_mutex.lock();
    DIR *dir;
    struct dirent *ent;
    string keysDir("conf/keys/");
    if ((dir = opendir(keysDir.c_str())) != NULL)
    {
        string suffix = ".PBLCkey";
        while ((ent = readdir(dir)) != NULL)
        {
            string filename = ent->d_name;
            if (!has_suffix(filename, suffix)) continue;
            string name = filename.substr(0, filename.size() - suffix.size());
            if (name.length()<1 || localPrivateKeys.count(name)!=0) continue;
            string pathAndName;
            pathAndName.append(keysDir);
            pathAndName.append(filename);
            CryptoPP::RSA::PublicKey *publicKey = new CryptoPP::RSA::PublicKey();
            CryptoPP::ByteQueue byteQueue;
            CryptoPP::FileSource file(pathAndName.c_str(), true);
            file.TransferTo(byteQueue);
            byteQueue.MessageEnd();
            publicKey->Load(byteQueue);
            string str;
            if (toString(*publicKey, str) && str.size() > 2)
            {
                localPublicKeys.insert(pair<string, string>(name, str));
                localPublicKeysNames.insert(pair<string, string>(str, name));
                if (localPublicKeysIDs.count(name)!=1)
                {
                    localPblcKeysUnreg.push_back(name);
                }
            }
        }
        closedir(dir);
    }
    keys_mutex.unlock();
}

void KeysHandling::tryToStorePublicKey(string &name, string str)
{
    if (str.length()<3) return;

    keys_mutex.lock();
    if (localPublicKeys.count(name)!=0)
    {
        keys_mutex.unlock();
        return;
    }
    localPublicKeys.insert(pair<string, string>(name, str));
    localPublicKeysNames.insert(pair<string, string>(str, name));
    if (localPublicKeysIDs.count(name)!=1)
    {
        localPblcKeysUnreg.push_back(name);
    }
    keys_mutex.unlock();

    // save to local file
    string fileName("conf/keys/"+name+".PBLCkey");
    CryptoPP::ByteQueue bq;
    bq.Put((const byte*)str.c_str(), str.length());
    CryptoPP::FileSink fileSink(fileName.c_str());
    bq.CopyTo(fileSink);
    fileSink.MessageEnd();
}

TNtrNr KeysHandling::getNotary(string *publicKeyStr)
{
    if (publicKeyStr==nullptr) return TNtrNr(0,0);
    keys_mutex.lock();
    if (publicKeyStr->length()<3 || notaryNumByPublicKey.count(*publicKeyStr)!=1)
    {
        keys_mutex.unlock();
        return TNtrNr(0,0);
    }
    keys_mutex.unlock();
    return notaryNumByPublicKey[*publicKeyStr];
}

string KeysHandling::getName(string &publicKeyStr)
{
    keys_mutex.lock();
    if (publicKeyStr.length()<3 || localPublicKeysNames.count(publicKeyStr)!=1)
    {
        keys_mutex.unlock();
        return "";
    }
    keys_mutex.unlock();
    return localPublicKeysNames[publicKeyStr];
}

string KeysHandling::getNameByID(CompleteID &id)
{
    keys_mutex.lock();
    if (id.getNotary()<=0 || localPublicKeysNamesByID.count(id)!=1)
    {
        // try to find an alias
        if (nameByAliasId.count(id)!=1)
        {
            keys_mutex.unlock();
            return "";
        }
        keys_mutex.unlock();
        return nameByAliasId[id];
    }
    keys_mutex.unlock();
    return localPublicKeysNamesByID[id];
}

bool KeysHandling::addNameAndId(string &name, CompleteID &id)
{
    if (id.getNotary()<=0 || name.length()<1) return false;
    keys_mutex.lock();
    localPublicKeysIDs.insert(pair<string, CompleteID>(name, id));
    localPublicKeysNamesByID.insert(pair<CompleteID, string>(id, name));
    keys_mutex.unlock();
    return true;
}

bool KeysHandling::setId(string name, CompleteID id)
{
    keys_mutex.lock();
    if (name.length()<1 || localPublicKeysIDs.count(name)!=0 || id.getNotary()<=0)
    {
        keys_mutex.unlock();
        return false;
    }
    localPublicKeysIDs.insert(pair<string, CompleteID>(name, id));
    localPublicKeysNamesByID.insert(pair<CompleteID, string>(id, name));
    localPblcKeysUnreg.remove(name);
    size_t oldUnregSize = localPrvtKeysUnreg.size();
    localPrvtKeysUnreg.remove(name);
    size_t newUnregSize = localPrvtKeysUnreg.size();
    keys_mutex.unlock();
    savePublicKeysIDs();
    if (newUnregSize < oldUnregSize)
    {
        string *msg = new string("Account \"");
        msg->append(name.c_str());
        msg->append("\" was successfully registered.\nPlease verify its details under \"Show account info\".");
        Fl::awake((void*)msg);
    }
    return true;
}

bool KeysHandling::addIdAlias(string name, CompleteID id)
{
    keys_mutex.lock();
    if (name.length()<1 || localPublicKeysIDs.count(name)<=0
            || id.getNotary()<=0 || localPublicKeysNamesByID.count(id)>0)
    {
        keys_mutex.unlock();
        return false;
    }
    if (nameByAliasId.count(id)<1)
    {
        nameByAliasId.insert(pair<CompleteID, string>(id, name));
        if (keyIdAliases.count(name)<1)
        {
            CIDsSet *s = new CIDsSet();
            keyIdAliases[name]=s;
        }
        keyIdAliases[name]->add(id);
        keys_mutex.unlock();
        return true;
    }
    keys_mutex.unlock();
    return false;
}

bool KeysHandling::updateValidity(string name, unsigned long long validUntil)
{
    keys_mutex.lock();
    if (name.length()<1 || localPublicKeysIDs.count(name)!=0)
    {
        keys_mutex.unlock();
        return false;
    }
    if (validity.count(name)<1) validity[name]=validUntil;
    else validity[name] = min(validUntil, validity[name]);
    keys_mutex.unlock();
    return true;
}

void KeysHandling::savePublicKeysIDs()
{
    keys_mutex.lock();
    // load public keys ids into json object
    json out;
    map<string, CompleteID>::iterator it;
    for (it=localPublicKeysIDs.begin(); it!=localPublicKeysIDs.end(); ++it)
    {
        json ids;
        // add first id
        ids.push_back(it->second.to27Char());
        // ad aliases
        if (keyIdAliases.count(it->first) > 0)
        {
            set<CompleteID, CompleteID::CompareIDs>* idsSet = keyIdAliases[it->first]->getSetPointer();
            set<CompleteID, CompleteID::CompareIDs>::iterator it2;
            for (it2 = idsSet->begin(); it2!=idsSet->end(); ++it2)
            {
                CompleteID id = *it2;
                ids.push_back(id.to27Char());
            }
        }
        out[it->first] = ids;
    }

    // try to save to json file
    string fileNameJ("conf/keysIDs.json");
    ofstream fJ(fileNameJ.c_str());
    if (!fJ.good())
    {
        fJ.close();
        keys_mutex.unlock();
        return;
    }
    fJ << setw(4) << out << endl;
    fJ.close();
    keys_mutex.unlock();
}

CompleteID KeysHandling::getLatestT2eId()
{
    keys_mutex.lock();
    CompleteID out = latestT2eId;
    keys_mutex.unlock();
    return out;
}

CompleteID KeysHandling::getType2EntryIdFirst(TNtrNr totalNotaryNr)
{
    keys_mutex.lock();
    if (type2entryIdsFirst.count(totalNotaryNr)<=0)
    {
        keys_mutex.unlock();
        return CompleteID();
    }
    CompleteID out = type2entryIdsFirst[totalNotaryNr];
    keys_mutex.unlock();
    return out;
}

CompleteID KeysHandling::getNotApplTerminationId(TNtrNr totalNotaryNr)
{
    keys_mutex.lock();
    if (terminationIds.count(totalNotaryNr)<=0)
    {
        keys_mutex.unlock();
        return CompleteID();
    }
    CompleteID out = terminationIds[totalNotaryNr];
    keys_mutex.unlock();
    return out;
}

// keys_mutex must be locked for this
void KeysHandling::setLatestT2eId(CompleteID id)
{
    if (id>latestT2eId) latestT2eId = id;
}

// keys_mutex must be locked for this
CompleteID KeysHandling::getLastT2eIdFirst()
{
    if (type2entryIdsFirst.size()<=0)
    {
        return CompleteID();
    }
    CompleteID out = type2entryIdsFirst.rbegin()->second;
    return out;
}

// keys_mutex must be locked before this (and will remain locked after)
bool KeysHandling::addNotaryKeyEtc(string &publicKeyStr, CompleteID type2entryId, CompleteID terminationId)
{
    if (type2entryId.getNotary()<=0 || terminationId.getNotary()<=0) return false;
    const unsigned short lineage = ((Type1Entry*)type1entry)->getLineage(type2entryId.getTimeStamp());
    // calculate new notaryNr
    TNtrNr notaryNr;
    if (type2entryIdsFirst.size()>0)
    {
        TNtrNr oldNotaryNr = type2entryIdsFirst.rbegin()->first;
        if (lineage == oldNotaryNr.getLineage()) notaryNr = TNtrNr(lineage, oldNotaryNr.getNotaryNr()+1);
        else
        {
            if (lineage <= oldNotaryNr.getLineage())
            {
                return false;
            }
            notaryNr = TNtrNr(lineage, ((Type1Entry*)type1entry)->getMaxActing(lineage)+1);
        }
    }
    else
    {
        notaryNr = TNtrNr(lineage, ((Type1Entry*)type1entry)->getMaxActing(lineage)+1);
    }
    // check and add
    if (!notaryNr.isGood() || notariesKeys.count(notaryNr)!=0)
    {
        return false;
    }
    CryptoPP::RSA::PublicKey* key = new CryptoPP::RSA::PublicKey();
    CryptoPP::ByteQueue bq;
    bq.Put((const byte*)publicKeyStr.c_str(), publicKeyStr.length());
    key->Load(bq);
    notariesKeys.insert(pair<TNtrNr, CryptoPP::RSA::PublicKey*>(notaryNr, key));
    notariesKeysStr.insert(pair<TNtrNr, string>(notaryNr, publicKeyStr));
    type2entryIdsFirst.insert(pair<TNtrNr, CompleteID>(notaryNr, type2entryId));
    terminationIds.insert(pair<TNtrNr, CompleteID>(notaryNr, terminationId));
    notaryNumByPublicKey.insert(pair<string, TNtrNr>(publicKeyStr, notaryNr));
    appointedNotariesNum[lineage]++;
    return true;
}

// keys_mutex MIGHT have been locked before this
CryptoPP::RSA::PublicKey* KeysHandling::getNotaryPubKey(TNtrNr totalNotaryNr, const bool wasAlreadyLocked)
{
    if (!wasAlreadyLocked) keys_mutex.lock();
    if (!totalNotaryNr.isGood() || notariesKeys.count(totalNotaryNr)!=1)
    {
        if (!wasAlreadyLocked) keys_mutex.unlock();
        return nullptr;
    }
    CryptoPP::RSA::PublicKey* out = notariesKeys[totalNotaryNr];
    if (!wasAlreadyLocked) keys_mutex.unlock();
    return out;
}

void KeysHandling::lock()
{
    keys_mutex.lock();
}

void KeysHandling::unlock()
{
    keys_mutex.unlock();
}

unsigned long KeysHandling::getAppointedNotariesNum(unsigned short lineage)
{
    keys_mutex.lock();
    if (lineage < 1 || lineage > ((Type1Entry*)type1entry)->latestLin())
    {
        keys_mutex.unlock();
        return 0;
    }
    unsigned long out = appointedNotariesNum[lineage];
    keys_mutex.unlock();
    return out;
}

string* KeysHandling::getNotaryPubKeyStr(TNtrNr totalNotaryNr)
{
    keys_mutex.lock();
    if (!totalNotaryNr.isGood() || notariesKeysStr.count(totalNotaryNr)!=1)
    {
        keys_mutex.unlock();
        return nullptr;
    }
    keys_mutex.unlock();
    return &notariesKeysStr[totalNotaryNr];
}

bool KeysHandling::has_suffix(const std::string &str, const std::string &suffix)
{
    return (str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

KeysHandling::~KeysHandling()
{
    // delete notaries keys
    map<TNtrNr, CryptoPP::RSA::PublicKey*>::iterator it;
    for (it=notariesKeys.begin(); it!=notariesKeys.end(); ++it)
    {
        delete it->second;
    }
    notariesKeys.clear();
    notariesKeysStr.clear();

    // delete local private keys
    map<string, CryptoPP::RSA::PrivateKey*>::iterator it2;
    for (it2=localPrivateKeys.begin(); it2!=localPrivateKeys.end(); ++it2)
    {
        delete it2->second;
    }
    localPrivateKeys.clear();

    // delete keyIdAliases
    map<string, CIDsSet*>::iterator it3;
    for (it3=keyIdAliases.begin(); it3!=keyIdAliases.end(); ++it3)
    {
        delete it3->second;
    }
    keyIdAliases.clear();

    // delete rest
    localPrvtKeysUnreg.clear();
    localPublicKeysIDs.clear();
    localPublicKeys.clear();
    localPublicKeysNames.clear();
    localPblcKeysUnreg.clear();
    localPublicKeysNamesByID.clear();
}

CryptoPP::RSA::PrivateKey* KeysHandling::getPrivateKey(string &name)
{
    keys_mutex.lock();
    if (localPrivateKeys.count(name)!=1)
    {
        keys_mutex.unlock();
        return nullptr;
    }
    keys_mutex.unlock();
    return localPrivateKeys[name];
}

string* KeysHandling::getPublicKeyStr(string &name)
{
    keys_mutex.lock();
    if (localPublicKeys.count(name)!=1)
    {
        keys_mutex.unlock();
        return nullptr;
    }
    keys_mutex.unlock();
    return &localPublicKeys[name];
}

string KeysHandling::getLocalPrvtKeyUnreg()
{
    if (localPrvtKeysUnreg.size()==0) return "";
    keys_mutex.lock();
    auto it = localPrvtKeysUnreg.begin();
    advance(it, rand() % localPrvtKeysUnreg.size());
    string out = *it;
    keys_mutex.unlock();
    return out;
}

void KeysHandling::loadPrivateKeysNames(list<string> &namesList)
{
    keys_mutex.lock();
    map<string, CryptoPP::RSA::PrivateKey*>::iterator it;
    for (it=localPrivateKeys.begin(); it!=localPrivateKeys.end(); ++it)
    {
        if (localPublicKeysIDs.count(it->first)==1)
        {
            namesList.push_back(it->first);
        }
    }
    keys_mutex.unlock();
}

CompleteID KeysHandling::getLatestID(string &name)
{
    keys_mutex.lock();
    if (localPublicKeysIDs.count(name)!=1)
    {
        keys_mutex.unlock();
        return CompleteID();
    }
    // check if largest alias is larger
    CompleteID out = localPublicKeysIDs[name];
    if (keyIdAliases.count(name)==1 && !keyIdAliases[name]->isEmpty()) out = keyIdAliases[name]->last();
    keys_mutex.unlock();
    return out;
}

CompleteID KeysHandling::getInitialID(string &name)
{
    keys_mutex.lock();
    if (localPublicKeysIDs.count(name)!=1)
    {
        keys_mutex.unlock();
        return CompleteID();
    }
    CompleteID out = localPublicKeysIDs[name];
    keys_mutex.unlock();
    return out;
}

void KeysHandling::loadPublicKeysNames(list<string> &namesList)
{
    keys_mutex.lock();
    map<string, CompleteID>::iterator it;
    for (it=localPublicKeysIDs.begin(); it!=localPublicKeysIDs.end(); ++it)
    {
        namesList.push_back(it->first);
    }
    keys_mutex.unlock();
}

string KeysHandling::getLocalPblcKeyUnreg()
{
    if (localPblcKeysUnreg.size()==0) return "";
    keys_mutex.lock();
    auto it = localPblcKeysUnreg.begin();
    advance(it, rand() % localPblcKeysUnreg.size());
    string out = *it;
    keys_mutex.unlock();
    return out;
}

Type1Entry* KeysHandling::getType1Entry()
{
    return ((Type1Entry*)type1entry);
}

bool KeysHandling::toString(CryptoPP::RSA::PublicKey &publicKey, string &str)
{
    CryptoPP::ByteQueue bqueue;
    publicKey.Save(bqueue);
    CryptoPP::byte runningByte;
    string out;
    size_t nr;
    do
    {
        nr = bqueue.Get(runningByte);
        if (nr != 1) break;
        out.push_back(runningByte);
    }
    while (nr == 1);
    if (out.size()<3) return false;
    str.append(out);
    return true;
}
