#include "ShowKeyInfoW.h"

KeysHandling* ShowKeyInfoW::keys;
list<Fl_Window*>* ShowKeyInfoW::subWins;

ShowKeyInfoW::ShowKeyInfoW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(580, 255, "Show account info"),
    internal(i), waitForDataThread(nullptr)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // choose account
    box0 = new Fl_Box(20,15,166,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Account (optional):");
    ch0 = new Fl_Choice(190,15,230,25);
    ch0->add(" ");
    list<string> publicKeysNames;
    keys->loadPublicKeysNames(publicKeysNames);
    list<string>::iterator it;
    for (it=publicKeysNames.begin(); it!=publicKeysNames.end(); ++it)
    {
        ch0->add(it->c_str());
    }
    ch0->take_focus();
    ch0->value(0);

    // key id
    box = new Fl_Box(20,45,166,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box->label("Key ID (optional):");
    inp = new Fl_Input(190,45,230,25);
    inp->textsize(10);
    inp->textfont(FL_COURIER);

    // request button
    but = new Fl_Button(450,45,110,25,"Request data");
    but->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 85, 540, 150, 5);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");
    b->link(linkRoutine);

    end();
    resizable(b);
}

ShowKeyInfoW::~ShowKeyInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
}

void ShowKeyInfoW::onRequest(Fl_Widget *w, void *d)
{
    ShowKeyInfoW *win = (ShowKeyInfoW*)d;
    // checks
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->rqstNum != 0)
    {
        fl_alert("Error: %s", "already waiting for data.");
        return;
    }
    if (win->ch0->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // get key id
    CompleteID keyId;
    string str = string(win->ch0->text());
    string strInp(win->inp->value());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), ' '), strInp.end());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), '-'), strInp.end());
    if (str.length()>0 && str.compare(" ")!=0)
    {
        if (strInp.length()>0)
        {
            fl_alert("Error: %s", "please choose between Account name and Key ID\nby deleting either one.");
            return;
        }
        keyId = win->keys->getInitialID(str);
    }
    else
    {
        // get key id from input field
        if (strInp.length()<=0)
        {
            fl_alert("Error: %s", "no Key ID supplied.");
            return;
        }
        if (strInp.length()>0 && strInp.length()!=27)
        {
            fl_alert("Error: %s", "supplied Key ID does not have an admissible length.");
            return;
        }
        keyId = CompleteID(strInp);
        if (keyId.getNotary()<=0)
        {
            fl_alert("Error: %s", "supplied Key ID does not have an admissible format.");
            return;
        }
    }

    // build and send request
    string rqst;
    bool result = win->requestBuilder->createIdInfoRqst(keyId, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create(win->waitForDataThread, NULL, ShowKeyInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowKeyInfoW::waitForDataRoutine(void *w)
{
    ShowKeyInfoW* win=(ShowKeyInfoW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    list<pair<Type12Entry*, CIDsSet*>>* entriesList = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        entriesList = win->msgProcessor->getProcessedRqst(requestNumber);
        rounds++;
    }
    while (entriesList == nullptr && rounds < 20 && win->internal->running);
    // display entries list
    if (entriesList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (entriesList->size()<=0 || entriesList->begin()->first->underlyingType()!=11)
    {
        win->b->value("<h3>No such key entry reported.</h3>");
        win->b->redraw();
    }
    else
    {
        Util u;
        // get keyId and related stuff
        list<pair<Type12Entry*, CIDsSet*>>::iterator it=entriesList->begin();
        Type12Entry* t12e = it->first;
        Type11Entry* t11e = (Type11Entry*) t12e->underlyingEntry();
        CompleteID firstId = it->second->first();
        string htmlTxt;
        // report time
        htmlTxt.append("Key registration time: ");
        htmlTxt.append(u.epochToStr(firstId.getTimeStamp()));
        htmlTxt.append("<br>\n");
        // report key id
        htmlTxt.append("Key Id: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(firstId.to27Char());
        string ownerName = win->keys->getNameByID(firstId);
        if (ownerName.length()==0)
        {
            htmlTxt.append(" </font> <a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(firstId.to27Char());
            htmlTxt.append("\">ASSIGN NAME</a><br>\n");
        }
        else
        {
            htmlTxt.append("</font> (");
            htmlTxt.append(ownerName);
            htmlTxt.append(")<br>\n");
        }
        // get other IDs and minimal validity
        it=entriesList->begin();
        set<CompleteID, CompleteID::CompareIDs> *idsList = it->second->getSetPointer();
        set<CompleteID, CompleteID::CompareIDs>::iterator idIt = idsList->begin();
        unsigned long c = 0;
        string otherIdsTxt;
        unsigned long long minValidity=ULLONG_MAX;
        while (idIt!=idsList->end() && it!=entriesList->end())
        {
            if (idIt==idsList->end())
            {
                ++it;
                if (it==entriesList->end()) break;
                // check that entry is consistent
                Type11Entry* t11e2 = (Type11Entry*) it->first->underlyingEntry();
                if (t11e->getPublicKey()->compare(*t11e2->getPublicKey()) != 0) continue;
                else
                {
                    idsList = it->second->getSetPointer();
                    idIt = idsList->begin();
                }
            }
            CompleteID aID = *idIt;
            if (c>0)
            {
                if (c==1) otherIdsTxt.append("Alternative IDs:<br><font face=\"courier\" size=\"2\">");
                else otherIdsTxt.append(",<br>\n");

                otherIdsTxt.append(aID.to27Char());
            }
            minValidity = min(minValidity, ((Type11Entry*) it->first->underlyingEntry())->getValidityDate());
            c++;
            ++idIt;
        }
        if (c>1) otherIdsTxt.append("</font><br>\n");
        // report validity
        htmlTxt.append("Valid until: ");
        htmlTxt.append(u.epochToStr(minValidity));
        htmlTxt.append("<br>\n");
        // report hash
        CryptoPP::SHA224 h;
        string hstr;
        CryptoPP::StringSource s(*(t11e->getPublicKey()), true, new CryptoPP::HashFilter(h,
                                 new CryptoPP::HexEncoder(new CryptoPP::StringSink(hstr), true, 7)));
        htmlTxt.append("Hash: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(hstr);
        htmlTxt.append("</font><br>\n");
        // report other IDs
        htmlTxt.append(otherIdsTxt);
        // print out
        win->b->value(htmlTxt.c_str());
        win->b->redraw();
    }
    usleep(200000);
    // clean up and exit
    win->msgProcessor->deleteOldRqst(requestNumber);
    win->rqstNum = 0;
    Fl::flush();
    pthread_exit(NULL);
}

const char* ShowKeyInfoW::linkRoutine(Fl_Widget *w, const char *uri)
{
    string str(uri);
    CompleteID keyId(str);
    if (keyId.getNotary()<=0)
    {
        fl_alert("Error: %s", "bad id.");
        return NULL;
    }
    // create NewAccountW
    NewAccountW *newAccountW = new NewAccountW(keys, keyId);
    subWins->insert(subWins->end(), (Fl_Window*)newAccountW);
    ((Fl_Window*)newAccountW)->show();
    // exit
    return NULL;
}

