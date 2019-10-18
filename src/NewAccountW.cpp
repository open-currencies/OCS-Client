#include "NewAccountW.h"

NewAccountW::NewAccountW(KeysHandling *k) : Fl_Window(330, 120, "Create new account"), keys(k)
{
    box = new Fl_Box(20,10,100,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->label("Name of new key pair / account:");
    inp = new Fl_Input(100,40,200,25);
    but = new Fl_Button(240,80,60,25,"Create");
    but->callback(NewAccountW::onCreate, (void*)this);
    end();
}

NewAccountW::NewAccountW(KeysHandling *k, CompleteID &id) : Fl_Window(380, 85, "Add account"), keys(k), keyID(id)
{
    box = new Fl_Box(10,10,100,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->label("Local name for account");

    boxId = new Fl_Box(188,11,100,25);
    boxId->labelfont(FL_COURIER);
    boxId->labelsize(10);
    boxId->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    string txt(" ");
    txt.append(keyID.to27Char());
    txt.append(":");
    boxId->copy_label(txt.c_str());

    inp = new Fl_Input(80,40,200,25);
    but = new Fl_Button(300,40,60,25,"Save");
    but->callback(NewAccountW::onAdd, (void*)this);
    end();
}

NewAccountW::~NewAccountW()
{
    //dtor
}

void NewAccountW::onAdd(Fl_Widget *w, void *d)
{
    NewAccountW *win = (NewAccountW*)d;
    const char * seq = win->inp->value();
    string str(seq);
    if (str.length() < 1)
    {
        fl_alert("Error: %s", "name too short.");
        return;
    }
    else if (str.length() > 23)
    {
        fl_alert("Error: %s", "name too long.");
        return;
    }
    else if (!isGood(str))
    {
        fl_alert("Error: %s", "only latin letters, digits and \"_\" are allowed.");
        return;
    }
    if (!win->keys->addNameAndId(str, win->keyID))
    {
        fl_alert("Error: %s", "could not add ID under this name.");
        return;
    }
    win->keys->savePublicKeysIDs();
    win->hide();
}

void NewAccountW::onCreate(Fl_Widget *w, void *d)
{
    NewAccountW *win = (NewAccountW*)d;
    const char * seq = win->inp->value();
    string str(seq);
    if (str.length() < 1)
    {
        fl_alert("Error: %s", "name too short.");
        return;
    }
    else if (str.length() > 23)
    {
        fl_alert("Error: %s", "name too long.");
        return;
    }
    else if (!isGood(str))
    {
        fl_alert("Error: %s", "only latin letters, digits and \"_\" are allowed.");
        return;
    }
    // check if name already used
    if (win->keys == nullptr)
    {
        fl_alert("Error: %s", "name not confirmed as free.");
        return;
    }
    if (win->keys->getPublicKeyStr(str) != nullptr || !win->keys->getInitialID(str).isZero())
    {
        fl_alert("Error: %s", "key name already reserved.");
        return;
    }
    // NOTE: for mingw: replace "stat" with "_stat" for struct and function
    string dirSuffix("conf/keys/");
    string namePr(dirSuffix+str+".PRVTkey");
    struct stat prInfo;
    if (stat(namePr.c_str(), &prInfo) == 0)
    {
        fl_alert("Error: %s", "such a key already exists in \"conf/keys/\".");
        return;
    }
    string namePub(dirSuffix+str+".PBLCkey");
    struct stat pubInfo;
    if (stat(namePub.c_str(), &pubInfo) == 0)
    {
        fl_alert("Error: %s", "such a key already exists in \"conf/keys/\".");
        return;
    }
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::InvertibleRSAFunction params;
    params.GenerateRandomWithKeySize(rng, 3072);
    CryptoPP::RSA::PrivateKey privateKey(params);
    CryptoPP::RSA::PublicKey publicKey(params);
    // save private key
    CryptoPP::ByteQueue queuePr;
    privateKey.Save(queuePr);
    CryptoPP::FileSink filePr(namePr.c_str());
    queuePr.CopyTo(filePr);
    filePr.MessageEnd();
    // save public key
    CryptoPP::ByteQueue queuePub;
    publicKey.Save(queuePub);
    CryptoPP::FileSink filePub(namePub.c_str());
    queuePub.CopyTo(filePub);
    filePub.MessageEnd();
    // reload private keys
    if (win->keys!=nullptr) win->keys->loadPrivateKeys();
    // report and exit
    string notice("Store it at secure locations only and make sure that it does not get lost.\n\n");
    notice.append("Do not allow others to obtain the above file unless you want\n");
    notice.append("someone to obtain full control over the respective account.");
    Fl::lock();
    fl_alert("IMPORTANT: The file %s \ncontaining the private key was successfully created.\n\n%s",
             namePr.c_str(), notice.c_str());
    Fl::unlock();
    win->hide();
}

bool NewAccountW::isGood(string &s)
{
    bool out=true;
    for (unsigned int i=0; i<s.length() && out; i++)
    {
        out = out && (isalpha(s[i]) || isdigit(s[i]) || (s[i] == '_'));
    }
    return out;
}
