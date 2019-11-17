#include "LiquiditiesHandling.h"

LiquiditiesHandling::LiquiditiesHandling()
{
    loadIDsFromFile(false);

    // load liquis from dir
    DIR *dir;
    struct dirent *ent;
    string liquisDir("conf/liquis/");
    if ((dir = opendir(liquisDir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            string filename = ent->d_name;
            string suffix = ".t5eJ";
            if (!has_suffix(filename, suffix))
            {
                suffix = ".t15eJ";
                if (!has_suffix(filename, suffix))
                {
                    suffix = ".t5e";
                    if (!has_suffix(filename, suffix))
                    {
                        suffix = ".t15e";
                        if (!has_suffix(filename, suffix)) continue;
                    }
                }
            }
            string name = filename.substr(0, filename.size() - suffix.size());
            if (name.length()<1 || liquis.count(name)!=0) continue;
            // create Type5Or15Entry
            Type5Or15Entry *entry;
            if (suffix.compare(".t5eJ") == 0 || suffix.compare(".t5e") == 0)
            {
                entry = (Type5Or15Entry*) new Type5Entry(liquisDir, filename);
            }
            else
            {
                entry = (Type5Or15Entry*) new Type15Entry(liquisDir, filename);
            }
            // add to lists
            if (entry->isGood())
            {
                // store liqui
                liquis.insert(pair<string, Type5Or15Entry*>(name, entry));
                // store name
                liquisNames.insert(pair<string, string>(*entry->getByteSeq(), name));
                // add to unregs
                if (liquisIDs.count(name)!=1)
                {
                    liquisUnreg.push_back(name);
                }
            }
            else delete entry;
        }
        closedir(dir);
    }
}

// NOTE: for mingw: replace "stat" with "_stat" for struct and function

void LiquiditiesHandling::loadIDsFromFile(bool fromDownload)
{
    liquis_mutex.lock();
    // try to load from json file
    string fileNameJ("conf/liquisIDs.json");
    if (fromDownload) fileNameJ = "conf/liquisIDs_download.json";
    struct stat fJInfo;
    if (stat(fileNameJ.c_str(), &fJInfo) != 0)
    {
        liquis_mutex.unlock();
        return;
    }
    ifstream fJ(fileNameJ.c_str());
    if (!fJ.good())
    {
        fJ.close();
        liquis_mutex.unlock();
        return;
    }
    json j;
    try
    {
        fJ >> j;
        fJ.close();
    }
    catch (const exception& e)
    {
        fJ.close();
        liquis_mutex.unlock();
        return;
    }

    // read data from liquisIDs
    for (json::iterator it = j.begin(); it != j.end(); ++it)
    {
        string name = it.key();
        if (name.length()<1 || liquisIDs.count(name)!=0) continue;
        string idStr = it.value();
        if (idStr.length()!=40 && idStr.length()!=27) continue;
        CompleteID id(idStr);
        if (id.getNotary()<=0 || liquisNamesById.count(id)!=0) continue;

        liquisIDs.insert(pair<string, CompleteID>(name, id));
        liquisNamesById.insert(pair<CompleteID, string>(id, name));
        liquisUnreg.remove(name);
    }
    liquis_mutex.unlock();
}

LiquiditiesHandling::~LiquiditiesHandling()
{
    // delete liquis
    map<string, Type5Or15Entry*>::iterator it;
    for (it=liquis.begin(); it!=liquis.end(); ++it)
    {
        delete it->second;
    }
    liquis.clear();

    // delete rest
    liquisIDs.clear();
    liquisNamesById.clear();
    liquisUnreg.clear();
    liquisNames.clear();
}

string LiquiditiesHandling::getName(string &entryStr)
{
    liquis_mutex.lock();
    if (entryStr.length()<3 || liquisNames.count(entryStr)!=1)
    {
        liquis_mutex.unlock();
        return "";
    }
    liquis_mutex.unlock();
    return liquisNames[entryStr];
}

string LiquiditiesHandling::getNameById(CompleteID id)
{
    liquis_mutex.lock();
    if (liquisNamesById.count(id)!=1)
    {
        liquis_mutex.unlock();
        return "";
    }
    liquis_mutex.unlock();
    return liquisNamesById[id];
}

Type5Or15Entry* LiquiditiesHandling::getLiqui(string &name)
{
    liquis_mutex.lock();
    if (name.length()<1 || liquis.count(name)!=1)
    {
        liquis_mutex.unlock();
        return nullptr;
    }
    liquis_mutex.unlock();
    return liquis[name];
}

CompleteID LiquiditiesHandling::getID(string &name)
{
    liquis_mutex.lock();
    if (liquisIDs.count(name)!=1)
    {
        liquis_mutex.unlock();
        return CompleteID();
    }
    liquis_mutex.unlock();
    return liquisIDs[name];
}

bool LiquiditiesHandling::addLiqui(string name, CompleteID id)
{
    liquis_mutex.lock();

    if (name.length()<1 || liquisIDs.count(name)!=0
            || id.getNotary()<=0 || liquisNamesById.count(id)!=0)
    {
        liquis_mutex.unlock();
        return false;
    }

    liquisIDs.insert(pair<string, CompleteID>(name, id));
    liquisNamesById.insert(pair<CompleteID, string>(id, name));
    liquisUnreg.remove(name);

    liquis_mutex.unlock();
    return true;
}

// liquis_mutex must be locked for this
size_t LiquiditiesHandling::numOfRegistered()
{
    return liquisIDs.size();
}

bool LiquiditiesHandling::setId(string name, CompleteID id)
{
    liquis_mutex.lock();
    if (name.length()<1 || liquisIDs.count(name)!=0 || id.getNotary()<=0)
    {
        liquis_mutex.unlock();
        return false;
    }
    liquisIDs.insert(pair<string, CompleteID>(name, id));
    liquisNamesById.insert(pair<CompleteID, string>(id, name));
    size_t oldUnregSize = liquisUnreg.size();
    liquisUnreg.remove(name);
    size_t newUnregSize = liquisUnreg.size();
    liquis_mutex.unlock();
    if (newUnregSize < oldUnregSize)
    {
        string *msg = new string("Liquidity \"");
        msg->append(name.c_str());
        msg->append("\" was successfully registered.\nPlease verify its details under \"Show liquidity info\".");
        Fl::awake((void*)msg);
    }
    saveIDs();
    return true;
}

bool LiquiditiesHandling::setLiqui(string &name, Type5Or15Entry* entry)
{
    if (entry==nullptr || (entry->getType() != 5 && entry->getType() != 15)) return false;
    liquis_mutex.lock();
    if (name.length()<1 || liquisIDs.count(name)<1 || liquis.count(name)>0)
    {
        liquis_mutex.unlock();
        return false;
    }
    liquis.insert(pair<string, Type5Or15Entry*>(name, entry));
    liquis_mutex.unlock();
    // save byteSequence to file
    string fileName("conf/liquis/"+name);
    if (entry->getType() == 5) fileName.append(".t5e");
    else fileName.append(".t15e");
    ofstream outfile(fileName.c_str(), ios::binary);
    outfile << *entry->getByteSeq();
    outfile.close();
    return true;
}

void LiquiditiesHandling::loadKnownLiquidities(list<string> &liquisList)
{
    liquis_mutex.lock();
    map<string, CompleteID>::iterator it;
    for (it=liquisIDs.begin(); it!=liquisIDs.end(); ++it)
    {
        liquisList.push_back(it->first);
    }
    liquis_mutex.unlock();
}

string LiquiditiesHandling::getLiquiUnreg()
{
    if (liquisUnreg.size()==0) return "";
    liquis_mutex.lock();
    auto it = liquisUnreg.begin();
    advance(it, rand() % liquisUnreg.size());
    string out = *it;
    liquis_mutex.unlock();
    return out;
}

void LiquiditiesHandling::saveIDs()
{
    liquis_mutex.lock();
    // load liquis ids into json object
    json out;
    map<string, CompleteID>::iterator it;
    for (it=liquisIDs.begin(); it!=liquisIDs.end(); ++it)
    {
        out[it->first] = it->second.to27Char();
    }
    // try to save to json file
    string fileNameJ("conf/liquisIDs.json");
    ofstream fJ(fileNameJ.c_str());
    if (!fJ.good())
    {
        fJ.close();
        liquis_mutex.unlock();
        return;
    }
    fJ << setw(4) << out << endl;
    fJ.close();
    liquis_mutex.unlock();
}

bool LiquiditiesHandling::has_suffix(string &str, string &suffix)
{
    return (str.size() >= suffix.size() &&
            str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0);
}
