#ifndef LIQUIDITIESHANDLING_H
#define LIQUIDITIESHANDLING_H
#include <string>
#include <list>
#include <map>
#include <mutex>
#include "CompleteID.h"
#include <sys/stat.h>
#include <dirent.h>
#include "Type5Entry.h"
#include "Type15Entry.h"
#include "Type5Or15Entry.h"

using namespace std;
using json = nlohmann::json;

class LiquiditiesHandling
{
public:
    LiquiditiesHandling();
    ~LiquiditiesHandling();
    CompleteID getID(string &name);
    string getName(string &entryStr);
    string getNameById(CompleteID id);
    Type5Or15Entry* getLiqui(string &name);
    bool setLiqui(string &name, Type5Or15Entry* entry);
    bool addLiqui(string name, CompleteID id);
    bool setId(string name, CompleteID id);
    size_t numOfRegistered();
    void saveIDs();
    string getLiquiUnreg();
    void loadKnownLiquidities(list<string> &liquisList);
    void loadIDsFromFile(bool fromDownload);
    mutex liquis_mutex;
protected:
private:

    // currencies and obligations
    map<string, CompleteID> liquisIDs; // the key here is the name and the value the CID
    map<CompleteID, string, CompleteID::CompareIDs> liquisNamesById; // the key here is the CID and the value the name
    // information for unregistered liquis:
    list<string> liquisUnreg; // the values are the names (unregistered liquis)
    map<string, Type5Or15Entry*> liquis; // the key here is the name and the value the entry
    map<string, string> liquisNames; // the key here is the entry as string and the value the name

    static bool has_suffix(string &str, string &suffix);
};

#endif // LIQUIDITIESHANDLING_H
