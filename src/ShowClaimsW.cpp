#include "ShowClaimsW.h"

ShowClaimsW::ShowClaimsW(InternalThread *i) : Fl_Window(600, 420, "Show liquidity movements"), internal(i), waitForListThread(nullptr)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;
    account = "";
    liqui = "";

    // choose account
    group1 = new Fl_Group(20, 10, 455, 25);
    box0 = new Fl_Box(20,10,196,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Account:");
    ch0 = new Fl_Choice(220,10,220,25);
    list<string> publicKeysNames;
    keys->loadPublicKeysNames(publicKeysNames);
    list<string>::iterator it;
    for (it=publicKeysNames.begin(); it!=publicKeysNames.end(); ++it)
    {
        ch0->add(it->c_str());
    }
    ch0->callback(onChoice, (void*)this);
    ch0->take_focus();
    ch0->value(0);
    dum1 = new Fl_Box(450,10,5,25);
    dum1->hide();
    group1->resizable(dum1);
    group1->end();

    // choose currency
    group2 = new Fl_Group(20, 40, 455, 25);
    box1 = new Fl_Box(20,40,196,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation:");
    ch1 = new Fl_Choice(220,40,220,25);
    list<string> liquisNames;
    liquis->loadKnownLiquidities(liquisNames);
    list<string>::iterator it1;
    for (it1=liquisNames.begin(); it1!=liquisNames.end(); ++it1)
    {
        ch1->add(it1->c_str());
    }
    ch1->callback(onChoice, (void*)this);
    ch1->take_focus();
    ch1->value(0);
    dum2 = new Fl_Box(450,40,5,25);
    dum2->hide();
    group2->resizable(dum2);
    group2->end();

    // set statement id
    group3 = new Fl_Group(20, 70, 455, 25);
    box3 = new Fl_Box(20,70,196,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("maximal entry time:");
    inp = new Fl_Input(220,70,220,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);
    Util u;
    inp->value(u.epochToStr2(requestBuilder->currentSyncTimeInMs()+1000*60*60).c_str());
    dum3 = new Fl_Box(450,70,5,25);
    dum3->hide();
    group3->resizable(dum3);
    group3->end();

    // new state button
    but2 = new Fl_Button(450,25,130,25,"Create new state");
    but2->callback(ShowClaimsW::onNewClaim, (void*)this);

    // request button
    but1 = new Fl_Button(470,70,110,25,"Request data");
    but1->callback(ShowClaimsW::onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 110, 560, 290, 0);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");

    end();
    resizable(b);
}

ShowClaimsW::~ShowClaimsW()
{
    if (waitForListThread!=nullptr) delete waitForListThread;
}

void ShowClaimsW::onChoice(Fl_Widget *w, void *d)
{
    ShowClaimsW *win = (ShowClaimsW*)d;
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    Fl_Choice *ch = (Fl_Choice*)w;
    string str(ch->text());

    if (ch == win->ch0 && str.compare(win->account) != 0)
    {
        // update key
        win->account=str;
        CompleteID keyID = win->keys->getInitialID(win->account);
        win->internal->addIdOfInterest(keyID);
        if (win->ch1->text()!=nullptr) str = string(win->ch1->text());
        else str = "";
        // update liqui
        if (str.compare(win->liqui) != 0)
        {
            win->liqui=str;
            CompleteID liquiID = win->liquis->getID(win->liqui);
            win->internal->addIdOfInterest(liquiID);
        }
        // add pair
        win->internal->addAccountPairOfInterest(win->account, win->liqui);
    }
    else if (ch == win->ch1 && str.compare(win->liqui) != 0)
    {
        // update liqui
        win->liqui=str;
        CompleteID liquiID = win->liquis->getID(win->liqui);
        win->internal->addIdOfInterest(liquiID);
        // update key
        if (win->ch0->text()!=nullptr) str = string(win->ch0->text());
        else str = "";
        if (str.compare(win->account) != 0)
        {
            win->account=str;
            CompleteID keyID = win->keys->getInitialID(win->account);
            win->internal->addIdOfInterest(keyID);
        }
        win->internal->addAccountPairOfInterest(win->account, win->liqui);
    }
}

void ShowClaimsW::onRequest(Fl_Widget *w, void *d)
{
    ShowClaimsW *win = (ShowClaimsW*)d;
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
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr || win->inp->value()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // update account
    string str = string(win->ch0->text());
    if (str.compare(win->account) != 0)
    {
        win->account=str;
        CompleteID keyID = win->keys->getInitialID(win->account);
        win->internal->addIdOfInterest(keyID);
    }
    // update liqui
    str = string(win->ch1->text());
    if (str.compare(win->liqui) != 0)
    {
        win->liqui=str;
        CompleteID liquiID = win->liquis->getID(win->liqui);
        win->internal->addIdOfInterest(liquiID);
    }
    // add pair
    win->internal->addAccountPairOfInterest(win->account, win->liqui);
    // get max time and build respective claimId
    str = string(win->inp->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no maximal time supplied.");
        return;
    }
    unsigned long long maxTime = 0;
    try
    {
        Util u;
        maxTime = u.str2ToEpoch(str)+499;
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad maximal time supplied.");
        return;
    }
    if (maxTime<=0)
    {
        fl_alert("Error: %s", "bad maximal time supplied.");
        return;
    }
    CompleteID claimId(ULONG_MAX, ULLONG_MAX, maxTime);
    Util u;
    win->inp->value(u.epochToStr2(maxTime).c_str());
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createClaimsInfoRqst(win->account, win->liqui, claimId, 14, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->logInfo("ShowClaimsW::onRequest: sendRequest ...");
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    win->logInfo("ShowClaimsW::onRequest: starting waitForListThread ...");
    if (win->rqstNum <= 0) return;
    if (win->waitForListThread!=nullptr) delete win->waitForListThread;
    win->waitForListThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForListThread, NULL, ShowClaimsW::waitForListRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForListThread));
}

void ShowClaimsW::onNewClaim(Fl_Widget *w, void *d)
{
    ShowClaimsW *win = (ShowClaimsW*)d;
    // checks and questions
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // update account
    string str = string(win->ch0->text());
    if (str.compare(win->account) != 0)
    {
        win->account=str;
        CompleteID keyID = win->keys->getInitialID(win->account);
        win->internal->addIdOfInterest(keyID);
    }
    if (win->keys->getPrivateKey(win->account)==nullptr)
    {
        fl_alert("Error: %s", "only possible for account owner.");
        return;
    }
    int answer = fl_choice("This will produce a new state even if no transactions took place.\nDo you want to procede?",
                           "Close", "No", "Yes");
    if (answer != 2) return;

    // update liqui
    str = string(win->ch1->text());
    if (str.compare(win->liqui) != 0)
    {
        win->liqui=str;
        CompleteID liquiID = win->liquis->getID(win->liqui);
        win->internal->addIdOfInterest(liquiID);
    }
    // add pair
    win->internal->addAccountPairOfInterest(win->account, win->liqui);
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createNextClaimRqst(win->account, win->liqui, rqst);
    if (result)
    {
        win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
        win->b->value("<h3></h3>");
        win->b->redraw();
        Util u;
        win->inp->value(u.epochToStr2(win->requestBuilder->currentSyncTimeInMs()+1000*60*60).c_str());
        fl_alert("Info: %s", "the creation of a new state was initiated.\nPlease request new data after a few seconds.");
        return;
    }
}

void* ShowClaimsW::waitForListRoutine(void *w)
{
    ShowClaimsW* win=(ShowClaimsW*) w;
    win->logInfo("ShowClaimsW::waitForListRoutine");
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    list<pair<Type12Entry*, CIDsSet*>>* claimsList = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        claimsList = win->msgProcessor->getProcessedRqst(requestNumber);
        rounds++;
    }
    while (claimsList == nullptr && rounds < 20 && win->internal->running);
    // display claims list
    if (claimsList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (claimsList->size()==0)
    {
        win->b->value("<h3>No statements reported.</h3>");
        win->b->redraw();
    }
    else
    {
        string htmlTxt;
        // build table head
        htmlTxt.append("<table border=2 bordercolor=\"#FFFFFF\"><tr>");
        htmlTxt.append("<td bgcolor=\"#ECECEC\" nowrap>Time</td>");
        htmlTxt.append("<td bgcolor=\"#ECECEC\" nowrap>State ID</td>");
        htmlTxt.append("<td bgcolor=\"#ECECEC\" nowrap>Amount <font size=\"2\">(Transferable)</font></td>");
        htmlTxt.append("<td bgcolor=\"#ECECEC\" nowrap>Transaction IDs</td>");
        htmlTxt.append("</tr>\n");
        // create rows with claims info
        Util u;
        list<pair<Type12Entry*, CIDsSet*>>::reverse_iterator it;
        for (it=claimsList->rbegin(); it!=claimsList->rend(); ++it)
        {
            Type14Entry* t14e = (Type14Entry*) it->first->underlyingEntry();
            CompleteID id = it->second->first();
            htmlTxt.append("<tr>");
            string tdStart("<td bgcolor=\"#AEED92\" ");
            if (t14e->outflowsDominate()) tdStart = string("<td bgcolor=\"#F9BBAE\" ");
            // date and time
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font size=\"2\">");
            htmlTxt.append(u.epochToStrWithBr(id.getTimeStamp()));
            htmlTxt.append("</font></td>");
            // claim ID
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font face=\"courier\" size=\"1\">");
            htmlTxt.append(u.splitInTheMiddle(id.to27Char()));
            htmlTxt.append("</font></td>");
            // amount
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"center\" valign=\"top\" nowrap>");
            htmlTxt.append(u.toString(t14e->getTotalAmount(), 2));
            // transferable
            htmlTxt.append(" <font size=\"2\">(");
            htmlTxt.append(u.toString(t14e->getTransferableAmount(), 2));
            htmlTxt.append(")</font></td>");
            // add transaction ids
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap>");
            unsigned short predsCount = t14e->getPredecessorsCount();
            for (unsigned short i=0; i<predsCount; i++)
            {
                unsigned short scenario = t14e->getScenario(i);
                CompleteID predId = t14e->getPredecessor(i);
                double impact = t14e->getImpact(i);
                if (impact>0 && scenario!=0) htmlTxt.append("+");
                htmlTxt.append(u.toString(impact, 2));
                htmlTxt.append(" from ");
                if (scenario==0)
                {
                    htmlTxt.append("previous state: ");
                }
                else if (scenario==1)
                {
                    htmlTxt.append("incoming transfer: ");
                }
                else if (scenario==2)
                {
                    htmlTxt.append("outgoing transfer: ");
                }
                else if (scenario==3)
                {
                    htmlTxt.append("notarization fees: ");
                }
                else if (scenario==4)
                {
                    htmlTxt.append("initial grant: ");
                }
                else if (scenario==5)
                {
                    htmlTxt.append("proposal forfeit: ");
                }
                else if (scenario==6)
                {
                    htmlTxt.append("stake deposit: ");
                }
                else if (scenario==7)
                {
                    htmlTxt.append("won thread: ");
                }
                else if (scenario==8)
                {
                    htmlTxt.append("stake reclaim: ");
                }
                else if (scenario==9)
                {
                    htmlTxt.append("approved proposal: ");
                }
                htmlTxt.append("<font face=\"courier\" size=\"1\">");
                htmlTxt.append(predId.to27Char());
                htmlTxt.append("</font>");
                htmlTxt.append("<br>\n");
            }
            if (predsCount==0)
            {
                htmlTxt.append("No previous states.\n");
            }
            htmlTxt.append("</td></tr>\n");
        }
        htmlTxt.append("</table>");

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

void ShowClaimsW::setLogger(Logger *l)
{
    log=l;
}

void ShowClaimsW::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void ShowClaimsW::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}
