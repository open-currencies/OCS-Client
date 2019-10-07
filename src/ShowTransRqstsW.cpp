#include "ShowTransRqstsW.h"

ShowTransRqstsW::ShowTransRqstsW(InternalThread *i) : Fl_Window(600, 420, "Show liquidity requests"), internal(i), waitForListThread(nullptr)
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
    box0 = new Fl_Box(20,10,196,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Account:");
    ch0 = new Fl_Choice(220,10,220,25);
    list<string> privateKeysNames;
    keys->loadPrivateKeysNames(privateKeysNames);
    list<string>::iterator it;
    for (it=privateKeysNames.begin(); it!=privateKeysNames.end(); ++it)
    {
        ch0->add(it->c_str());
    }
    ch0->callback(onChoice, (void*)this);
    ch0->take_focus();
    ch0->value(0);

    // choose currency
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

    // set maximal request time
    box3 = new Fl_Box(20,70,196,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("maximal request time:");
    inp = new Fl_Input(220,70,220,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);
    Util u;
    inp->value(u.epochToStr2(requestBuilder->currentSyncTimeInMs()+1000*60*60).c_str());

    // request button
    but1 = new Fl_Button(470,70,110,25,"Request data");
    but1->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 110, 560, 290, 0);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");

    end();
    resizable(b);
}

ShowTransRqstsW::~ShowTransRqstsW()
{
    if (waitForListThread!=nullptr) delete waitForListThread;
}

void ShowTransRqstsW::onChoice(Fl_Widget *w, void *d)
{
    ShowTransRqstsW *win = (ShowTransRqstsW*)d;
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
        if (win->ch1->text() != nullptr) str = string(win->ch1->text());
        else str="";
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
        if (win->ch0->text() != nullptr) str = string(win->ch0->text());
        else str="";
        if (str.compare(win->account) != 0)
        {
            win->account=str;
            CompleteID keyID = win->keys->getInitialID(win->account);
            win->internal->addIdOfInterest(keyID);
        }
        win->internal->addAccountPairOfInterest(win->account, win->liqui);
    }
}

void ShowTransRqstsW::onRequest(Fl_Widget *w, void *d)
{
    ShowTransRqstsW *win = (ShowTransRqstsW*)d;
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

    // get max time and build respective transId
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
    CompleteID transId(ULONG_MAX, ULLONG_MAX, maxTime);

    // build and send request
    string rqst;
    bool result = win->requestBuilder->createTransRqstsInfoRqst(win->account, win->liqui, transId, 14, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForListThread!=nullptr) delete win->waitForListThread;
    win->waitForListThread = new pthread_t();
    if (pthread_create(win->waitForListThread, NULL, ShowTransRqstsW::waitForListRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForListThread));
}

void* ShowTransRqstsW::waitForListRoutine(void *w)
{
    ShowTransRqstsW* win=(ShowTransRqstsW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    list<pair<Type12Entry*, CIDsSet*>>* transactionsList = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        transactionsList = win->msgProcessor->getProcessedRqst(requestNumber);
        rounds++;
    }
    while (transactionsList == nullptr && rounds < 20 && win->internal->running);
    // display claims list
    if (transactionsList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (transactionsList->size()==0)
    {
        win->b->value("<h3>No transfer requests reported.</h3>");
        win->b->redraw();
    }
    else
    {
        string htmlTxt;
        // build table head
        htmlTxt.append("<table border=2 bordercolor=\"#FFFFFF\"><tr>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Creation Time</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Transfer Request ID</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Requested Amount</td>");
        htmlTxt.append("</tr>\n");
        // create rows with claims info
        Util u;
        list<pair<Type12Entry*, CIDsSet*>>::iterator it;
        for (it=transactionsList->begin(); it!=transactionsList->end(); ++it)
        {
            Type10Entry* t10e = (Type10Entry*) it->first->underlyingEntry();
            CompleteID id = it->second->first();
            htmlTxt.append("<tr>");
            string tdStart("<td bgcolor=\"#F0F0F0\" ");
            // date and time
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font size=\"2\">");
            htmlTxt.append(u.epochToStr(id.getTimeStamp()));
            htmlTxt.append("</font></td>");
            // Trans.Request ID
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font face=\"courier\" size=\"2\">");
            htmlTxt.append(id.to27Char());
            htmlTxt.append("</font></td>");
            // amount
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"center\" valign=\"top\" nowrap>");
            htmlTxt.append(u.toString(t10e->getRequestedAmount(), 2));
            htmlTxt.append("</td></tr>\n");
        }
        htmlTxt.append("</table>");

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
