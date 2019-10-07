#include "AcceptOfferW.h"

#define discountTimeBufferInMs 60000

AcceptOfferW::AcceptOfferW(InternalThread *i, string lR, double aR, string lO, double aO, CIDsSet *targetT10EId, CompleteID oID)
    : Fl_Window(500, 210, "Exchange liquidity"), internal(i), liquiR(lR), liquiO(lO), amountR(aR), amountO(aO), targetId(targetT10EId), ownerId(oID)
{
    Util u;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    source="";

    // choose source account
    box0 = new Fl_Box(20,10,236,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Source account:");
    ch0 = new Fl_Choice(260,10,220,25);
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

    // display requested currency
    box1 = new Fl_Box(20,40,236,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation to send:");
    ch1 = new Fl_Choice(260,40,220,25);
    ch1->add(liquiR.c_str());
    ch1->callback(onChoice, (void*)this);
    ch1->take_focus();
    ch1->value(0);

    // display requested amount
    box3 = new Fl_Box(20,70,236,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to transfer:");
    inpR = new Fl_Input(260,70,220,25);
    inpR->textsize(10);
    inpR->value(u.toString(amountR, 2).c_str());
    inpR->readonly(1);

    // display offered currency
    box2 = new Fl_Box(20,100,236,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Currency/Obligation to receive:");
    ch2 = new Fl_Choice(260,100,220,25);
    ch2->add(liquiO.c_str());
    ch2->callback(onChoice, (void*)this);
    ch2->take_focus();
    ch2->value(0);

    unsigned long long currentTimeInMs = requestBuilder->currentSyncTimeInMs();
    if (currentTimeInMs == 0)
    {
        end();
        fl_alert("Error: %s", "synchronization with notary failed.");
        hide();
        return;
    }

    // display offered amount
    Type5Or15Entry *t5Or15e = liquis->getLiqui(liquiO);
    unsigned long long discountEndDate = currentTimeInMs;
    if (amountO * t5Or15e->getRate() < 0) discountEndDate+=discountTimeBufferInMs;
    else discountEndDate-=discountTimeBufferInMs;
    double amountOdiscounted = amountO * u.getDiscountFactor(targetId->first().getTimeStamp(), discountEndDate, t5Or15e->getRate());
    box4 = new Fl_Box(20,130,236,25);
    box4->labelfont(FL_COURIER);
    box4->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box4->label("Amount to receive:");
    inpO = new Fl_Input(260,130,220,25);
    inpO->textsize(10);
    inpO->value(u.toString(amountOdiscounted, 2).c_str());
    inpO->readonly(1);

    but1 = new Fl_Button(20,170,145,25,"Delete offer");
    but1->callback(AcceptOfferW::onSend, (void*)this);
    but2 = new Fl_Button(335,170,145,25,"Exchange liquidity");
    but2->callback(AcceptOfferW::onSend, (void*)this);

    callback(AcceptOfferW::onClose, (void*)this);
    end();

    but1->deactivate();
    but2->deactivate();
    dataStatusClear=false;

    // update source
    if (ch0->text() != nullptr) source=string(ch0->text());
    CompleteID keyID = keys->getInitialID(source);
    internal->addIdOfInterest(keyID);
    // add liquiR
    CompleteID liquiID = liquis->getID(liquiR);
    internal->addIdOfInterest(liquiID);
    // add pair
    internal->addAccountPairOfInterest(source, liquiR);
    // add liquiO
    liquiID = liquis->getID(liquiO);
    internal->addIdOfInterest(liquiID);
    // add pair
    internal->addAccountPairOfInterest(source, liquiO);

    checkThreadRunning=true;
    if (pthread_create(&checkDataThread, NULL, checkDataRoutine, (void*) this) < 0)
    {
        checkThreadRunning=false;
        return;
    }
    pthread_detach(checkDataThread);
}

AcceptOfferW::~AcceptOfferW()
{
    delete targetId;
}

void AcceptOfferW::onSend(Fl_Widget *w, void *d)
{
    Util u;
    AcceptOfferW *win = (AcceptOfferW*)d;

    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->ch0->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // update source
    string str = string(win->ch0->text());
    if (str.compare(win->source) != 0)
    {
        win->source=str;
        CompleteID keyID = win->keys->getInitialID(win->source);
        win->internal->addIdOfInterest(keyID);
    }
    // add pair
    win->internal->addAccountPairOfInterest(win->source, win->liquiR);

    // build request
    string rqst;
    bool result = false;
    Fl_Button *but = (Fl_Button*)w;
    unsigned long notaryNr = win->connection->getNotaryNr();
    if (but == win->but2)
    {
        // check that win->source has sufficient funds in win->liqui
        Type14Entry *t14e =  win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiR);
        if (t14e==nullptr) result = false;
        else
        {
            if (t14e->getTransferableAmount() < win->amountR)
            {
                fl_alert("Error: %s", "insufficient liquidity.");
                delete t14e;
                return;
            }
            // build request
            CompleteID lastTargetId = win->targetId->last();
            result = win->requestBuilder->createOfferAcceptRqst(win->liquiR, win->amountR, lastTargetId, win->amountO, win->source, notaryNr, rqst);
            if (result)
            {
                int answer = fl_choice("Are you sure you want to transfer %s units of liquidity %s\nfrom account %s\nto satisfy exchange offer %s?",
                                       "Close", "No", "Yes", u.toString(win->amountR, 2).c_str(), win->liquiR.c_str(),
                                       win->source.c_str(), lastTargetId.to27Char().c_str());
                if (answer != 2)
                {
                    delete t14e;
                    return;
                }
                // rebuild request
                rqst="";
                result = win->requestBuilder->createOfferAcceptRqst(win->liquiR, win->amountR, lastTargetId, win->amountO, win->source, notaryNr, rqst);
            }

            delete t14e;
        }
    }
    else if (but == win->but1)
    {
        Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiO);
        unsigned long long currentTimeInMs = win->requestBuilder->currentSyncTimeInMs();
        if (t14e==nullptr || currentTimeInMs==0) result = false;
        else
        {
            Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liquiO);
            unsigned long long discountEndDate = currentTimeInMs;
            if (win->amountO * t5Or15e->getRate() < 0) discountEndDate+=discountTimeBufferInMs;
            else discountEndDate-=discountTimeBufferInMs;
            double discountedAmount = win->amountO * u.getDiscountFactor(win->targetId->first().getTimeStamp(), discountEndDate, t5Or15e->getRate());
            // build request
            CompleteID lastTargetId = win->targetId->last();
            result = win->requestBuilder->createOfferCancelRqst(win->liquiO, discountedAmount, win->source, lastTargetId, notaryNr, rqst);
            if (result)
            {
                int answer = fl_choice("Are you sure you want to cancel exchange offer\n%s?",
                                       "Close", "No", "Yes", lastTargetId.to27Char().c_str());
                currentTimeInMs = win->requestBuilder->currentSyncTimeInMs();
                if (answer != 2 || currentTimeInMs==0)
                {
                    delete t14e;
                    return;
                }
                // rebuild request
                rqst="";
                discountEndDate = currentTimeInMs;
                if (win->amountO * t5Or15e->getRate() < 0) discountEndDate+=discountTimeBufferInMs;
                else discountEndDate-=discountTimeBufferInMs;
                discountedAmount = win->amountO * u.getDiscountFactor(win->targetId->first().getTimeStamp(), discountEndDate, t5Or15e->getRate());
                result = win->requestBuilder->createOfferCancelRqst(win->liquiO, discountedAmount, win->source, lastTargetId, notaryNr, rqst);
            }
            delete t14e;
        }
    }

    if (result)
    {
        win->connection->sendRequest(rqst);
        fl_alert("Info: %s", "a transaction was sent for notarization.\nPlease check your account state before performing other transactions.");
        win->checkThreadRunning=false;
        win->hide();
    }
    else
    {
        fl_alert("Error: %s", "insufficient data to process request.");
    }
}

void AcceptOfferW::onChoice(Fl_Widget *w, void *d)
{
    AcceptOfferW *win = (AcceptOfferW*)d;

    if (!win->connection->connectionEstablished())
    {
        win->but1->deactivate();
        win->but2->deactivate();
        win->dataStatusClear=false;
        fl_alert("Error: %s", "no connection established.");
        return;
    }

    Fl_Choice *ch = (Fl_Choice*)w;
    string str(ch->text());

    if (ch == win->ch0 && str.compare(win->source) != 0)
    {
        // update key
        win->source=str;
        CompleteID keyID = win->keys->getInitialID(win->source);
        win->internal->addIdOfInterest(keyID);
        // add pair
        win->internal->addAccountPairOfInterest(win->source, win->liquiR);
        // add pair
        win->internal->addAccountPairOfInterest(win->source, win->liquiO);
    }
    win->but1->deactivate();
    win->but2->deactivate();
    win->dataStatusClear=false;
}

void AcceptOfferW::onClose(Fl_Widget *w, void *d)
{
    AcceptOfferW *win = (AcceptOfferW*)d;
    win->checkThreadRunning=false;
    win->hide();
}

void* AcceptOfferW::checkDataRoutine(void *w)
{
    AcceptOfferW* win=(AcceptOfferW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        string offerOwner = win->keys->getNameByID(win->ownerId);
        if (win->source.compare(offerOwner)==0)
        {
            Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liquiO);
            if (t5Or15e!=nullptr)
            {
                Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiO);
                if (t14e!=nullptr)
                {
                    win->dataStatusClear=true;
                    win->but1->activate();
                    delete t14e;
                }
                else win->dataStatusClear = false;
            }
            else win->dataStatusClear = false;
        }
        else
        {
            Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liquiR);
            if (t5Or15e!=nullptr)
            {
                Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiR);
                if (t14e!=nullptr)
                {
                    win->dataStatusClear=true;
                    win->but2->activate();
                    delete t14e;
                }
                else win->dataStatusClear = false;
            }
            else win->dataStatusClear = false;
        }

        usleep(500000);
    }
    pthread_exit(NULL);
}
