#include "ShowTransInfoW.h"

mutex ShowTransInfoW::winByTransIdMutex;
map<string, ShowTransInfoW*> ShowTransInfoW::winByTransId;
map<string, CompleteID> ShowTransInfoW::ownerByTransId;
map<string, CIDsSet*> ShowTransInfoW::idsByTransId;

ShowTransInfoW::ShowTransInfoW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(550, 255, "Show transaction info"),
    subWins(s), internal(i), requestEntry(nullptr), waitForDataThread(nullptr)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // transaction id
    box = new Fl_Box(20,15,136,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box->label("Transaction ID:");
    inp = new Fl_Input(170,15,220,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);

    // request button
    but = new Fl_Button(420,15,110,25,"Request data");
    but->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 55, 510, 180, 5);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");
    b->link(linkRoutine);

    end();
    resizable(b);
}

ShowTransInfoW::~ShowTransInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
}

void ShowTransInfoW::onRequest(Fl_Widget *w, void *d)
{
    ShowTransInfoW *win = (ShowTransInfoW*)d;
    if (win->requestEntry!=nullptr)
    {
        delete win->requestEntry;
        win->requestEntry=nullptr;
    }
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
    // get trans id
    string str(win->inp->value());
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '-'), str.end());
    if (str.length()>0 && str.length()!=27)
    {
        fl_alert("Error: %s", "supplied Transaction ID does not have an admissible length.");
        return;
    }
    CompleteID transId(str);
    if (transId.getNotary()<=0)
    {
        fl_alert("Error: %s", "supplied Transaction ID does not have an admissible format.");
        return;
    }
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createIdInfoRqst(transId, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForDataThread, NULL, ShowTransInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowTransInfoW::waitForDataRoutine(void *w)
{
    ShowTransInfoW* win=(ShowTransInfoW*) w;
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
    else if (transactionsList->size()<=0 || transactionsList->begin()->first->underlyingType()!=10)
    {
        win->b->value("<h3>No such transaction reported.</h3>");
        win->b->redraw();
    }
    else
    {
        Util u;
        // get transId and related stuff
        list<pair<Type12Entry*, CIDsSet*>>::iterator it=transactionsList->begin();
        Type12Entry *t12e = it->first;
        CompleteID ownerID = t12e->pubKeyID();
        Type10Entry* t10e = (Type10Entry*) t12e->underlyingEntry();
        CIDsSet *transIDs = it->second;
        CompleteID transId = transIDs->first();
        // get collecting claim (if existent)
        ++it;
        Type14Entry* t14e = nullptr;
        CompleteID collectorId;
        if (transactionsList->size()>1 && it->first->underlyingType()==14)
        {
            t14e = (Type14Entry*) it->first->underlyingEntry();
            // check that t14e is consistent
            if (!t14e->collectsFromTransaction(*transIDs)) t14e = nullptr;
            else collectorId = it->second->first();
        }
        // get connected transfer (if existent)
        list<pair<Type12Entry*, CIDsSet*>>::reverse_iterator rit=transactionsList->rbegin();
        Type10Entry* t10e2 = nullptr;
        CompleteID connectedTransId;
        if (transactionsList->size()>1 && rit->first->underlyingType()==10)
        {
            t10e2 = (Type10Entry*) rit->first->underlyingEntry();
            // check that t10e2 is consistent
            if (t10e2->satisfiesOfferOrRequest(*transIDs) || t10e->satisfiesOfferOrRequest(*rit->second))
            {
                connectedTransId = rit->second->first();
            }
            else t10e2 = nullptr;
        }
        string htmlTxt;
        // store data for unsatisfied requests
        string latestIdStr;
        if (t10e->getRequestedAmount()>0 && t10e2 == nullptr)
        {
            if (win->requestEntry!=nullptr)
            {
                delete win->requestEntry;
                win->requestEntry=nullptr;
            }
            win->requestEntry = (Type10Entry*) t10e->createCopy();
            latestIdStr = transactionsList->begin()->second->last().to27Char();

            winByTransIdMutex.lock();
            if (winByTransId.count(latestIdStr)<1)
            {
                winByTransId.insert(pair<string, ShowTransInfoW*>(latestIdStr, win));
                ownerByTransId.insert(pair<string, CompleteID>(latestIdStr, ownerID));
                idsByTransId.insert(pair<string, CIDsSet*>(latestIdStr, transactionsList->begin()->second->createCopy()));
            }
            else
            {
                winByTransId[latestIdStr]=win;
                ownerByTransId[latestIdStr]=ownerID;
                delete idsByTransId[latestIdStr];
                idsByTransId[latestIdStr]=transactionsList->begin()->second->createCopy();
            }
            winByTransIdMutex.unlock();
        }
        // report transaction type
        htmlTxt.append("<b>Transaction type: ");
        if (t10e->getTransferAmount()==0)
        {
            if (t10e2 == nullptr)
            {
                htmlTxt.append("transfer request <a style=\"text-decoration: none\" href=\"");
                htmlTxt.append(latestIdStr);
                htmlTxt.append("\">SATISFY THIS REQUEST</a>");
            }
            else htmlTxt.append("satisfied transfer request");
        }
        else if (t10e->getRequestedAmount()==0)
        {
            if (t10e->getTarget().isZero()) htmlTxt.append("liquidity deletion");
            else htmlTxt.append("liquidity transfer");
        }
        else if (t10e2 == nullptr && t14e == nullptr)
        {
            htmlTxt.append("exchange offer <a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(latestIdStr);
            htmlTxt.append("\">ACCEPT THIS OFFER</a>");
        }
        else if (t10e2 != nullptr)
        {
            htmlTxt.append("satisfied exchange offer");
        }
        else if (t14e != nullptr)
        {
            htmlTxt.append("former exchange offer");
        }
        htmlTxt.append("</b><br>&nbsp;<br>\n");
        // report time
        htmlTxt.append("Transaction time: ");
        htmlTxt.append(u.epochToStr(transId.getTimeStamp()));
        htmlTxt.append("<br>\n");
        // report transaction id
        htmlTxt.append("Transaction Id: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(transId.to27Char());
        htmlTxt.append("</font><br>\n");
        // report signatory
        htmlTxt.append("Signatory Id: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(ownerID.to27Char());
        string ownerName = win->keys->getNameByID(ownerID);
        if (ownerName.length()==0) htmlTxt.append("</font><br>\n");
        else
        {
            htmlTxt.append("</font> (");
            htmlTxt.append(ownerName);
            htmlTxt.append(")<br>\n");
        }
        // report liquidity
        htmlTxt.append("Currency/Obligation Id: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(t10e->getCurrencyOrObl().to27Char());
        string liquiName = win->liquis->getNameById(t10e->getCurrencyOrObl());
        if (liquiName.length()==0) htmlTxt.append("</font><br>\n");
        else
        {
            htmlTxt.append("</font> (");
            htmlTxt.append(liquiName);
            htmlTxt.append(")<br>\n");
        }
        // transfer amount
        if (t10e->getTransferAmount()>0)
        {
            htmlTxt.append("Transfer amount: ");
            htmlTxt.append(u.toString(t10e->getTransferAmount(), 2));
            htmlTxt.append("<br>\n");
            CompleteID targetID = t10e->getTarget();
            if (t10e2 == nullptr && targetID.getNotary()>0)
            {
                // target id
                htmlTxt.append("Target Id: <font face=\"courier\" size=\"2\">");
                htmlTxt.append(targetID.to27Char());
                string targetName = win->keys->getNameByID(targetID);
                if (targetName.length()==0) targetName = win->liquis->getNameById(targetID);
                if (targetName.length()==0) htmlTxt.append("</font><br>\n");
                else
                {
                    htmlTxt.append("</font> (");
                    htmlTxt.append(targetName);
                    htmlTxt.append(")<br>\n");
                }
            }
            else if (t10e2 == nullptr && t10e->getRequestedAmount()>0)
            {
                // requested currency
                htmlTxt.append("requested Currency/Obligation Id: <font face=\"courier\" size=\"2\">");
                htmlTxt.append(targetID.to27Char());
                string liquiNameR = win->liquis->getNameById(targetID);
                if (liquiNameR.length()==0) htmlTxt.append("</font><br>\n");
                else
                {
                    htmlTxt.append("</font> (");
                    htmlTxt.append(liquiNameR);
                    htmlTxt.append(")<br>\n");
                }
            }
            else if (t10e2 != nullptr && t10e->getRequestedAmount()>0)
            {
                // received currency
                htmlTxt.append("received Currency/Obligation Id: <font face=\"courier\" size=\"2\">");
                htmlTxt.append(t10e2->getCurrencyOrObl().to27Char());
                string liquiNameR = win->liquis->getNameById(t10e2->getCurrencyOrObl());
                if (liquiNameR.length()==0) htmlTxt.append("</font><br>\n");
                else
                {
                    htmlTxt.append("</font> (");
                    htmlTxt.append(liquiNameR);
                    htmlTxt.append(")<br>\n");
                }
            }
        }
        // requested amount
        if (t10e->getRequestedAmount()>0)
        {
            if (t10e2 != nullptr && t10e->getTransferAmount()>0) htmlTxt.append("Received amount: ");
            else htmlTxt.append("Requested amount: ");
            htmlTxt.append(u.toString(t10e->getRequestedAmount(), 2));
            htmlTxt.append("<br>\n");
        }
        // collecting claim
        if (t14e != nullptr)
        {
            htmlTxt.append("Collecting State Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(collectorId.to27Char());
            htmlTxt.append("</font><br>\n");
        }
        // related transaction
        if (t10e2 != nullptr)
        {
            if (t10e->getRequestedAmount()==0)
            {
                htmlTxt.append("Transfer Request Id: <font face=\"courier\" size=\"2\">");
            }
            else if (t10e->getTransferAmount()==0)
            {
                htmlTxt.append("Liquidity Transfer Id: <font face=\"courier\" size=\"2\">");
            }
            else
            {
                htmlTxt.append("Counterparty Offer Id: <font face=\"courier\" size=\"2\">");
            }
            htmlTxt.append(connectedTransId.to27Char());
            htmlTxt.append("</font><br>\n");
        }
        // report other ids
        set<CompleteID, CompleteID::CompareIDs> *idsList = transactionsList->begin()->second->getSetPointer();
        set<CompleteID, CompleteID::CompareIDs>::iterator idIt = idsList->begin();
        bool firstIdToShow = true;
        for (++idIt; idIt!=idsList->end(); ++idIt)
        {
            if (firstIdToShow) htmlTxt.append("Alternative Transaction IDs:<br><font face=\"courier\" size=\"2\">");
            else htmlTxt.append(",<br>\n");
            CompleteID aID = *idIt;
            htmlTxt.append(aID.to27Char());
            firstIdToShow=false;
        }
        if (!firstIdToShow) htmlTxt.append("</font><br>\n");
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

const char* ShowTransInfoW::linkRoutine(Fl_Widget *w, const char *uri)
{
    string str(uri);

    winByTransIdMutex.lock();
    if (winByTransId.count(str)<1)
    {
        winByTransIdMutex.unlock();
        return NULL;
    }
    ShowTransInfoW* win = winByTransId[str];
    CompleteID ownerID = ownerByTransId[str];
    CIDsSet *ids = idsByTransId[str]->createCopy();
    winByTransIdMutex.unlock();

    CompleteID transId(str);
    if (transId.getNotary()<=0 || win->requestEntry==nullptr)
    {
        fl_alert("Error: %s", "bad id or entry.");
        return NULL;
    }

    // check requested amount
    double amount = win->requestEntry->getRequestedAmount();
    if (amount<=0)
    {
        fl_alert("Error: %s", "bad amount.");
        return NULL;
    }
    // check liquidity
    string liqui = win->liquis->getNameById(win->requestEntry->getCurrencyOrObl());
    if (liqui.length()<=0)
    {
        fl_alert("Error: %s", "unknown liquidity.");
        return NULL;
    }
    double offeredAmount = win->requestEntry->getTransferAmount();
    if (offeredAmount<=0)
    {
        // create transfer window
        TransferLiquiW *transferLiquiW = new TransferLiquiW(win->internal, liqui, amount, transId);
        win->subWins->insert(win->subWins->end(), (Fl_Window*)transferLiquiW);
        ((Fl_Window*)transferLiquiW)->show();
    }
    else
    {
        if (win->liquis->getLiqui(liqui)==nullptr)
        {
            fl_alert("Error: %s", "unknown liquidity offered.");
            return NULL;
        }
        // check requested liquidity
        string liquiR = win->liquis->getNameById(win->requestEntry->getTarget());
        if (liquiR.length()<=0)
        {
            fl_alert("Error: %s", "unknown liquidity (requested in offer).");
            return NULL;
        }
        // create accept offer window
        AcceptOfferW *acceptOfferW = new AcceptOfferW(win->internal, liquiR, amount, liqui, offeredAmount, ids, ownerID);
        win->subWins->insert(win->subWins->end(), (Fl_Window*)acceptOfferW);
        ((Fl_Window*)acceptOfferW)->show();
    }
    // clean up and exit
    delete win->requestEntry;
    win->requestEntry=nullptr;

    winByTransIdMutex.lock();
    if (winByTransId.count(str)>0) winByTransId.erase(str);
    if (ownerByTransId.count(str)>0) ownerByTransId.erase(str);
    if (idsByTransId.count(str)>0)
    {
        delete idsByTransId[str];
        idsByTransId.erase(str);
    }
    winByTransIdMutex.unlock();

    win->hide();
    return NULL;
}
