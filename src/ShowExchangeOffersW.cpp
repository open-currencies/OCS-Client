#include "ShowExchangeOffersW.h"

ShowExchangeOffersW::ShowExchangeOffersW(InternalThread *i) : Fl_Window(640, 450, "Show exchange offers"), internal(i), waitForListThread(nullptr)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;
    account = "";
    liquiO = "";
    liquiR = "";

    // choose account
    box0 = new Fl_Box(20,10,241,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Account (optional):");
    ch0 = new Fl_Choice(265,10,220,25);
    ch0->add(" ");
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
    box1 = new Fl_Box(20,40,241,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation offered:");
    ch1 = new Fl_Choice(265,40,220,25);
    ch1->add(" ");
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

    // choose currency
    box3 = new Fl_Box(20,70,241,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Currency/Obligation requested:");
    ch3 = new Fl_Choice(265,70,220,25);
    ch3->add(" ");
    for (it1=liquisNames.begin(); it1!=liquisNames.end(); ++it1)
    {
        ch3->add(it1->c_str());
    }
    ch3->callback(onChoice, (void*)this);
    ch3->take_focus();
    ch3->value(0);

    // choose requested amount
    box2 = new Fl_Box(20,100,241,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Amount requested:");
    box2->deactivate();
    ch2 = new Fl_Choice(265,100,220,25);
    ch2->add(" ");
    ch2->callback(onChoice, (void*)this);
    ch2->take_focus();
    ch2->value(0);
    ch2->textsize(10);
    ch2->deactivate();

    // request button
    but1 = new Fl_Button(510,100,110,25,"Request data");
    but1->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 140, 600, 290, 0);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");

    end();
    resizable(b);
}

ShowExchangeOffersW::~ShowExchangeOffersW()
{
    if (waitForListThread!=nullptr) delete waitForListThread;
}

void ShowExchangeOffersW::onChoice(Fl_Widget *w, void *d)
{
    ShowExchangeOffersW *win = (ShowExchangeOffersW*)d;
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    Fl_Choice *ch = (Fl_Choice*)w;
    string str(ch->text());

    if (ch == win->ch0 && str.compare(win->account) != 0)
    {
        if (str.compare(" ") == 0) win->account="";
        else
        {
            // update key
            win->account=str;
            CompleteID keyID = win->keys->getInitialID(win->account);
            win->internal->addIdOfInterest(keyID);
        }
        // update liquiO
        if (win->ch1->text()!=nullptr) str = string(win->ch1->text());
        else str="";
        if (str.compare(win->liquiO) != 0)
        {
            if (str.compare(" ") == 0) win->liquiO="";
            else
            {
                win->liquiO=str;
                CompleteID liquiID = win->liquis->getID(win->liquiO);
                win->internal->addIdOfInterest(liquiID);
            }
        }
        // update liquiR
        if (win->ch3->text()!=nullptr) str = string(win->ch3->text());
        else str="";
        if (str.compare(win->liquiR) != 0)
        {
            if (str.compare(" ") == 0) win->liquiR="";
            else
            {
                win->liquiR=str;
                CompleteID liquiID = win->liquis->getID(win->liquiR);
                win->internal->addIdOfInterest(liquiID);
            }
        }
    }
    else if (ch == win->ch3 && (str.compare(win->liquiR) != 0 || win->box2->active() == 0))
    {
        if (str.compare(win->liquiR) != 0)
        {
            if (str.compare(" ") == 0) win->liquiR="";
            else
            {
                // update liquiR
                win->liquiR=str;
                CompleteID liquiID = win->liquis->getID(win->liquiR);
                win->internal->addIdOfInterest(liquiID);
            }
        }
        // update list of amount ranges
        Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liquiR);
        if (t5Or15e!=nullptr)
        {
            double amount = t5Or15e->getMinTransferAmount();
            if (amount<=0) amount=0.01;
            Util u;
            win->ch2->clear();
            for (int j=0; j<20; j++)
            {
                string txt("from ");
                string decimal = u.toString(amount, 2);
                u.breakAfter3Digits(decimal);
                txt.append(decimal);
                txt.append(" to ");
                amount*=10;
                decimal = u.toString(amount, 2);
                u.breakAfter3Digits(decimal);
                txt.append(decimal);
                win->ch2->add(txt.c_str());
            }
            win->ch2->value(0);
            win->ch2->activate();
            win->box2->activate();
        }
        else
        {
            win->ch2->clear();
            win->ch2->add(" ");
            win->ch2->value(0);
            win->ch2->deactivate();
            win->box2->deactivate();
        }
        // update key
        if (win->ch0->text()!=nullptr) str = string(win->ch0->text());
        else str="";
        if (str.compare(win->account) != 0)
        {
            if (str.compare(" ") == 0) win->account="";
            else
            {
                win->account=str;
                CompleteID keyID = win->keys->getInitialID(win->account);
                win->internal->addIdOfInterest(keyID);
            }
        }
        // update liquiR
        if (win->ch1->text()!=nullptr) str = string(win->ch1->text());
        else str="";
        if (str.compare(win->liquiO) != 0)
        {
            if (str.compare(" ") == 0) win->liquiO="";
            else
            {
                win->liquiO=str;
                CompleteID liquiID = win->liquis->getID(win->liquiO);
                win->internal->addIdOfInterest(liquiID);
            }
        }
    }
}

void ShowExchangeOffersW::onRequest(Fl_Widget *w, void *d)
{
    ShowExchangeOffersW *win = (ShowExchangeOffersW*)d;
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
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr || win->ch3->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // update account
    string str = string(win->ch0->text());
    if (str.compare(win->account) != 0)
    {
        if (str.compare(" ") == 0) win->account="";
        else
        {
            win->account=str;
            CompleteID keyID = win->keys->getInitialID(win->account);
            win->internal->addIdOfInterest(keyID);
        }
    }
    // update liquiO
    str = string(win->ch1->text());
    if (str.compare(" ") == 0 || str.length()<=0)
    {
        fl_alert("Error: %s", "liquidity (offered) not specified.");
        return;
    }
    if (str.compare(win->liquiO) != 0)
    {
        win->liquiO=str;
        CompleteID liquiID = win->liquis->getID(win->liquiO);
        win->internal->addIdOfInterest(liquiID);
    }
    // update liquiR
    str = string(win->ch3->text());
    if (str.compare(" ") == 0 || str.length()<=0)
    {
        fl_alert("Error: %s", "liquidity (requested) not specified.");
        return;
    }
    if (str.compare(win->liquiR) != 0)
    {
        win->liquiR=str;
        CompleteID liquiID = win->liquis->getID(win->liquiR);
        win->internal->addIdOfInterest(liquiID);
    }
    if (win->liquiR.compare(win->liquiO) == 0)
    {
        fl_alert("Error: %s", "liquidities (offered and requested) cannot be the same.");
        return;
    }
    // get amount range
    if (win->box2->active() == 0 || win->ch2->active() == 0)
    {
        fl_alert("Error: %s", "amount range not specified.");
        return;
    }
    unsigned short rangeNum = (unsigned short) win->ch2->value();
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createExchangeOffersInfoRqst(win->account, win->liquiO, rangeNum, win->liquiR, 18, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForListThread!=nullptr) delete win->waitForListThread;
    win->waitForListThread = new pthread_t();
    if (pthread_create(win->waitForListThread, NULL, ShowExchangeOffersW::waitForListRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForListThread));
}

void* ShowExchangeOffersW::waitForListRoutine(void *w)
{
    ShowExchangeOffersW* win=(ShowExchangeOffersW*) w;
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
    Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liquiO);
    unsigned long long currentTimeInMs = win->requestBuilder->currentSyncTimeInMs();
    // display claims list
    if (transactionsList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (transactionsList->size()==0)
    {
        win->b->value("<h3>No exchange offers reported.</h3>");
        win->b->redraw();
    }
    else if (t5Or15e==nullptr)
    {
        win->b->value("<h3>Liquidity parameters unavailable.</h3>");
        win->b->redraw();
    }
    else if (currentTimeInMs==0)
    {
        win->b->value("<h3>Insufficient data reported.</h3>");
        win->b->redraw();
    }
    else
    {
        string htmlTxt;
        // build table head
        htmlTxt.append("<table border=2 bordercolor=\"#FFFFFF\"><tr>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Offer ID</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Offered Amount</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Requested Amount</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Ratio</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Creation Time</td>");
        htmlTxt.append("</tr>\n");
        // create rows with offer info
        Util u;
        list<pair<Type12Entry*, CIDsSet*>>::iterator it;
        for (it=transactionsList->begin(); it!=transactionsList->end(); ++it)
        {
            Type10Entry* t10e = (Type10Entry*) it->first->underlyingEntry();
            CompleteID id = it->second->first();
            htmlTxt.append("<tr>");
            string tdStart("<td bgcolor=\"#F0F0F0\" ");
            // Trans.Request ID
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font face=\"courier\" size=\"2\">");
            htmlTxt.append(id.to27Char());
            htmlTxt.append("</font></td>");
            // offered amount
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"center\" valign=\"top\" nowrap>");
            double discountedTransferAmount = t10e->getTransferAmount()
                                              * u.getDiscountFactor(id.getTimeStamp(), currentTimeInMs, t5Or15e->getRate());
            htmlTxt.append(u.toString(discountedTransferAmount, 2));
            htmlTxt.append("</td>\n");
            // requested amount
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"center\" valign=\"top\" nowrap>");
            htmlTxt.append(u.toString(t10e->getRequestedAmount(), 2));
            htmlTxt.append("</td>\n");
            // ratio
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"center\" valign=\"top\" nowrap>");
            htmlTxt.append(u.toString(discountedTransferAmount/t10e->getRequestedAmount(), 4));
            htmlTxt.append("</td>\n");
            // date and time
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font size=\"2\">");
            htmlTxt.append(u.epochToStr(id.getTimeStamp()));
            htmlTxt.append("</font></td></tr>");
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
