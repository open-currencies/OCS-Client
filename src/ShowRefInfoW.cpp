#include "ShowRefInfoW.h"

KeysHandling* ShowRefInfoW::keys;
list<Fl_Window*>* ShowRefInfoW::subWins;

ShowRefInfoW::ShowRefInfoW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(580, 330, "Show referee info"),
    internal(i), t5e(nullptr), waitForDataThread(nullptr)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // choose currency
    box0 = new Fl_Box(20,15,176,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Currency:");
    ch0 = new Fl_Choice(200,15,220,25);
    list<string> liquisNames;
    liquis->loadKnownLiquidities(liquisNames);
    list<string>::iterator it1;
    for (it1=liquisNames.begin(); it1!=liquisNames.end(); ++it1)
    {
        ch0->add(it1->c_str());
    }
    ch0->take_focus();
    ch0->value(0);

    // choose account
    box1 = new Fl_Box(20,45,176,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Account (optional):");
    ch1 = new Fl_Choice(200,45,220,25);
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

    // key id
    box = new Fl_Box(20,75,176,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box->label("Key ID (optional):");
    inp = new Fl_Input(200,75,220,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);

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

ShowRefInfoW::~ShowRefInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
}

void ShowRefInfoW::onRequest(Fl_Widget *w, void *d)
{
    ShowRefInfoW *win = (ShowRefInfoW*)d;
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
    win->t5e=nullptr;
    CompleteID liquiId;
    string str = string(win->ch0->text());
    if (str.length()>0 && str.compare(" ")!=0)
    {
        liquiId = win->liquis->getID(str);
        Entry* entry = win->liquis->getLiqui(str);
        if (entry == nullptr)
        {
            fl_alert("Error: %s", "insufficient data on currency.");
            return;
        }
        else if (entry->getType()!=5)
        {
            fl_alert("Error: %s", "not a currency.");
            return;
        }
        win->t5e = (Type5Entry*) entry;
    }
    else
    {
        fl_alert("Error: %s", "no currency choice supplied.");
        return;
    }
    win->liqui = win->liquis->getNameById(liquiId);

    // get key id
    CompleteID keyId;
    str = string(win->ch1->text());
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
    string key = win->keys->getNameByID(keyId);

    // build and send request
    string rqst;
    bool result = win->requestBuilder->createRefInfoRqst(key, win->liqui, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }

    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForDataThread, NULL, ShowRefInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowRefInfoW::waitForDataRoutine(void *w)
{
    ShowRefInfoW* win=(ShowRefInfoW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    string* refInfoStr = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        refInfoStr = win->msgProcessor->getProcessedRqstStr(requestNumber);
        rounds++;
    }
    while (refInfoStr == nullptr && rounds < 20 && win->internal->running);
    // display data etc.
    if (refInfoStr==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else
    {
        Util u;
        string htmlTxt;
        htmlTxt.append("<i>Information reported by notary:</i><br>&nbsp;<br>\n");
        // build RefInfo
        RefereeInfo* refInfo = new RefereeInfo(*refInfoStr);
        // display data
        CompleteID appointmentId = refInfo->getAppointmentId();
        if (appointmentId.getNotary()>0)
        {
            // report referee appointment
            string liquiCandidate = win->liquis->getNameById(appointmentId);
            if (win->liqui.compare(liquiCandidate) == 0)
                htmlTxt.append("Referee Appointment Id: <font face=\"courier\" size=\"2\">");
            else
                htmlTxt.append("Referee Appointment Id (decision thread): <font face=\"courier\" size=\"2\">");
            htmlTxt.append(appointmentId.to27Char());
            htmlTxt.append("</font><br>\n");
            // report tenure start
            unsigned long long tenureEnd = refInfo->getTenureEnd();
            unsigned long long tenureStart = win->t5e->getRefereeTenure();
            tenureStart*=1000;
            if (tenureStart<tenureEnd) tenureStart = tenureEnd - tenureStart;
            htmlTxt.append("Tenure start: ");
            htmlTxt.append(u.epochToStr(tenureStart));
            htmlTxt.append("<br>\n");
            // report tenure end
            htmlTxt.append("Tenure end: ");
            htmlTxt.append(u.epochToStr(tenureEnd));
            htmlTxt.append("<br>\n");
            // report liquidity approved
            htmlTxt.append("Liquidity approved so far: ");
            htmlTxt.append(u.toString(refInfo->getLiquidityCreated(), 2));
            htmlTxt.append("<br>\n");
            // report liquidity approval limit
            htmlTxt.append("Liquidity approval limit: ");
            htmlTxt.append(u.toString(win->t5e->getLiquidityCreationLimit(), 2));
            htmlTxt.append("<br>\n");
            // report refs appointed
            htmlTxt.append("Referees appointed so far: ");
            htmlTxt.append(to_string(refInfo->getRefsAppointed()));
            htmlTxt.append("<br>\n");
            // report refs appointment limit
            htmlTxt.append("Referees appointment limit: ");
            htmlTxt.append(to_string(win->t5e->getRefAppointmentLimit()));
            htmlTxt.append("<br>\n");
        }
        CompleteID pendingOrApprovedApplicationId = refInfo->getPendingOrApprovedApplicationId();
        if (pendingOrApprovedApplicationId.getNotary()>0)
        {
            htmlTxt.append("Pending or approved application Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(pendingOrApprovedApplicationId.to27Char());
            htmlTxt.append("</font><br>\n");
        }
        else if (appointmentId.getNotary()<=0)
        {
            htmlTxt.append("none");
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
