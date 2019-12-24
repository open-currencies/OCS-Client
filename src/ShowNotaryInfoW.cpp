#include "ShowNotaryInfoW.h"

KeysHandling* ShowNotaryInfoW::keys;
list<Fl_Window*>* ShowNotaryInfoW::subWins;

ShowNotaryInfoW::ShowNotaryInfoW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(580, 330, "Show notary info"),
    internal(i), publicKeyStr(nullptr), waitForDataThread(nullptr)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // key id
    box = new Fl_Box(23,15,176,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box->label("Notary nr. (e.g. 1,1):");
    inp = new Fl_Input(203,15,220,25);

    // choose account
    box1 = new Fl_Box(23,45,176,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Account (optional):");
    ch1 = new Fl_Choice(203,45,220,25);
    ch1->add(" ");
    list<string> publicKeysNames;
    keys->loadPublicKeysNames(publicKeysNames);
    list<string>::iterator it;
    for (it=publicKeysNames.begin(); it!=publicKeysNames.end(); ++it)
    {
        ch1->add(it->c_str());
    }
    ch1->take_focus();
    ch1->value(0);

    // choose currency
    box0 = new Fl_Box(23,75,176,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Liquidity (optional):");
    ch0 = new Fl_Choice(203,75,220,25);
    ch0->add(" ");
    list<string> liquisNames;
    liquis->loadKnownLiquidities(liquisNames);
    list<string>::iterator it1;
    for (it1=liquisNames.begin(); it1!=liquisNames.end(); ++it1)
    {
        ch0->add(it1->c_str());
    }
    ch0->take_focus();
    ch0->value(0);

    // request button
    but = new Fl_Button(450,75,110,25,"Request data");
    but->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 110, 540, 200, 5);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");

    end();
    resizable(b);
}

ShowNotaryInfoW::~ShowNotaryInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
}

void ShowNotaryInfoW::onRequest(Fl_Widget *w, void *d)
{
    ShowNotaryInfoW *win = (ShowNotaryInfoW*)d;
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
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // get liqui id
    CompleteID liquiId;
    string str = string(win->ch0->text());
    if (str.length()>0 && str.compare(" ")!=0) liquiId = win->liquis->getID(str);
    string liqui = win->liquis->getNameById(liquiId);

    // get public key
    string *publicKeyStr = nullptr;
    str = string(win->ch1->text());
    string strInp(win->inp->value());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), ' '), strInp.end());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), '-'), strInp.end());
    if (str.length()>0 && str.compare(" ")!=0)
    {
        if (strInp.length()>0)
        {
            fl_alert("Error: %s", "please choose between Account name and Total notary number\nby deleting either one.");
            return;
        }
        publicKeyStr = win->keys->getPublicKeyStr(str);
    }
    else
    {
        // get notary from input field
        if (strInp.length()<=0)
        {
            fl_alert("Error: %s", "no Total notary number supplied.");
            return;
        }
        size_t pos = strInp.find(",");
        if (pos == string::npos)
        {
            fl_alert("Error: %s", "supplied Total notary number does not have an admissible format.");
            return;
        }
        string lineageStr = strInp.substr(0, pos);
        string notnumStr = strInp.substr(pos+1, strInp.length()-pos-1);
        unsigned short lineage = (unsigned short) stoul(lineageStr);
        unsigned long notnum = stoul(notnumStr);
        TNtrNr totalNotaryNr(lineage,notnum);
        publicKeyStr = win->keys->getNotaryPubKeyStr(totalNotaryNr);
    }
    if (publicKeyStr == nullptr)
    {
        fl_alert("Error: %s", "notary could not be found.");
        return;
    }

    // build and send request
    string rqst;
    bool result = false;
    if (liqui.length()>0)
    {
        result = win->requestBuilder->createNotaryInfoRqst(publicKeyStr, liqui, rqst);
    }
    else
    {
        result = win->requestBuilder->createNotaryInfoRqst(publicKeyStr, rqst);
    }
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    else
    {
        fl_alert("Error: %s", "unable to create request.");
        return;
    }

    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForDataThread, NULL, ShowNotaryInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    win->publicKeyStr = publicKeyStr;
    pthread_detach(*(win->waitForDataThread));
}

void* ShowNotaryInfoW::waitForDataRoutine(void *w)
{
    ShowNotaryInfoW* win=(ShowNotaryInfoW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    string* notaryInfoStr = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        notaryInfoStr = win->msgProcessor->getProcessedRqstStr(requestNumber);
        rounds++;
    }
    while (notaryInfoStr == nullptr && rounds < 20 && win->internal->running);
    // display data etc.
    if (notaryInfoStr==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (win->publicKeyStr == nullptr)
    {
        win->b->value("<h3>Error.</h3>");
        win->b->redraw();
    }
    else
    {
        TNtrNr totalNotaryNr = win->keys->getNotary(win->publicKeyStr);
        string keyName = win->keys->getName(*win->publicKeyStr);
        Util u;
        string htmlTxt;
        // display data
        if (keyName.length()>0)
        {
            // Public Key id
            CompleteID pubKeyId = win->keys->getInitialID(keyName);
            htmlTxt.append("Public Key Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(pubKeyId.to27Char());
            htmlTxt.append("</font> (");
            htmlTxt.append(keyName);
            htmlTxt.append(")<br>\n");
        }
        if (!totalNotaryNr.isZero())
        {
            // report total notary number
            htmlTxt.append("Total notary number: ");
            htmlTxt.append(totalNotaryNr.toString());
            htmlTxt.append("<br>\n");
            // report termination id
            CompleteID terminationID = win->keys->getNotApplTerminationId(totalNotaryNr);
            if (terminationID.getNotary()>0)
            {
                htmlTxt.append("Approval Id (decision thread): <font face=\"courier\" size=\"2\">");
                htmlTxt.append(terminationID.to27Char());
                htmlTxt.append("</font><br>\n");
            }
            Type1Entry *t1e = win->keys->getType1Entry();
            // report tenure start
            unsigned long long tenureStart = t1e->getNotaryTenureStartCorrected(totalNotaryNr.getLineage(), totalNotaryNr.getNotaryNr());
            CompleteID type2EntryId = win->keys->getType2EntryIdFirst(totalNotaryNr);
            if (type2EntryId.getNotary()>0) tenureStart = max(tenureStart, type2EntryId.getTimeStamp());
            htmlTxt.append("Tenure start: ");
            htmlTxt.append(u.epochToStr(tenureStart));
            htmlTxt.append("<br>\n");
            // report tenure end
            unsigned long long tenureEnd = t1e->getNotaryTenureEndCorrected(totalNotaryNr.getLineage(), totalNotaryNr.getNotaryNr());
            htmlTxt.append("Tenure end: ");
            htmlTxt.append(u.epochToStr(tenureEnd));
            htmlTxt.append("<br>\n");
        }
        htmlTxt.append("<i>Information reported by corresponding notary:</i><br>\n");
        // build NotaryInfo
        NotaryInfo* notaryInfo = new NotaryInfo(*notaryInfoStr);
        if (notaryInfo!=nullptr)
        {
            CompleteID pubKeyId = notaryInfo->getPubKeyId();
            if (pubKeyId.getNotary()>0 && keyName.length()<=0)
            {
                htmlTxt.append("Public Key Id: <font face=\"courier\" size=\"2\">");
                htmlTxt.append(pubKeyId.to27Char());
                htmlTxt.append("</font><br>\n");
            }
            if (totalNotaryNr.isZero())
            {
                // report termination id
                CompleteID applicationID = notaryInfo->getPendingOrApprovedApplicationId();
                if (applicationID.getNotary()>0)
                {
                    htmlTxt.append("Pending application Id: <font face=\"courier\" size=\"2\">");
                    htmlTxt.append(applicationID.to27Char());
                    htmlTxt.append("</font><br>\n");
                }
                else if (notaryInfo->getPubKeyId().getNotary()<=0 || keyName.length()>0)
                {
                    htmlTxt.append("no notary information available");
                }
            }
            else
            {
                // report initiated threads
                htmlTxt.append("Initiated pending threads: ");
                htmlTxt.append(to_string(notaryInfo->getInitiatedThreadsNum()));
                htmlTxt.append("<br>\n");
                // report currency specific info
                CompleteID currencyId = notaryInfo->getLiquidityOfInterestId();
                if (currencyId.getNotary()>0)
                {
                    htmlTxt.append("Liquidity Id of interest: <font face=\"courier\" size=\"2\">");
                    htmlTxt.append(currencyId.to27Char());
                    htmlTxt.append("</font><br>\n");
                    // report fees
                    double fees = notaryInfo->getCollectedFees();
                    htmlTxt.append("Collected fees in liquidity: ");
                    htmlTxt.append(u.toString(fees, 2));
                    htmlTxt.append("<br>\n");
                }
                // report outgoing shares
                map<unsigned long, double>* outgoing = notaryInfo->getOutgoingShares();
                if (outgoing->size()>0)
                {
                    if (outgoing->count(totalNotaryNr.getNotaryNr()) > 0)
                    {
                        htmlTxt.append("share to keep: ");
                        htmlTxt.append(u.toString(outgoing->at(totalNotaryNr.getNotaryNr()), 3));
                        htmlTxt.append("<br>\n");
                    }
                    map<unsigned long, double>::iterator it;
                    for (it=outgoing->begin(); it!=outgoing->end(); ++it)
                    {
                        if (it->first == totalNotaryNr.getNotaryNr()) continue;
                        htmlTxt.append("share going to notary ");
                        htmlTxt.append(to_string(it->first));
                        htmlTxt.append(": ");
                        htmlTxt.append(u.toString(it->second, 3));
                        htmlTxt.append("<br>\n");
                    }
                }
                // report incoming shares
                map<unsigned long, double>* incoming = notaryInfo->getIncomingShares();
                if (incoming->size()>0)
                {
                    map<unsigned long, double>::iterator it;
                    for (it=incoming->begin(); it!=incoming->end(); ++it)
                    {
                        htmlTxt.append("share to receive from notary ");
                        htmlTxt.append(to_string(it->first));
                        htmlTxt.append(": ");
                        htmlTxt.append(u.toString(it->second, 3));
                        htmlTxt.append("<br>\n");
                    }
                }
            }
            delete notaryInfo;
        }
        // print out
        win->b->value(htmlTxt.c_str());
        win->b->redraw();
    }
    usleep(200000);
    // clean up and exit
    Fl::flush();
    win->msgProcessor->deleteOldRqst(requestNumber);
    win->rqstNum = 0;
    return NULL;
}
