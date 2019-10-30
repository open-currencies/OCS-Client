#include "TransferLiquiW.h"

TransferLiquiW::TransferLiquiW(InternalThread *i, Logger *l) : Fl_Window(430, 180, "Transfer liquidity"), internal(i), targetId(CompleteID()), fixedAmount(0), log(l)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    source="";
    liqui="";

    // choose source account
    box0 = new Fl_Box(20,10,166,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Source account:");
    ch0 = new Fl_Choice(190,10,220,25);
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
    box1 = new Fl_Box(20,40,166,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation:");
    ch1 = new Fl_Choice(190,40,220,25);
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

    // choose target account
    box2 = new Fl_Box(20,70,166,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Target account:");
    ch2 = new Fl_Choice(190,70,220,25);
    list<string> publicKeysNames;
    keys->loadPublicKeysNames(publicKeysNames);
    list<string>::iterator it2;
    for (it2=publicKeysNames.begin(); it2!=publicKeysNames.end(); ++it2)
    {
        ch2->add(it2->c_str());
    }
    ch2->callback(onChoice, (void*)this);
    ch2->take_focus();
    ch2->value(0);

    // set amount
    box3 = new Fl_Box(20,100,226,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to be transferred:");
    inp = new Fl_Input(280,100,130,25);

    but1 = new Fl_Button(20,140,145,25,"Send new liquidity");
    but1->callback(TransferLiquiW::onSend, (void*)this);
    but2 = new Fl_Button(265,140,145,25,"Send from account");
    but2->callback(TransferLiquiW::onSend, (void*)this);

    callback(TransferLiquiW::onClose, (void*)this);
    end();

    but1->deactivate();
    but2->deactivate();
    dataStatusClear=false;

    // update source
    if (ch0->text() != nullptr) source=string(ch0->text());
    CompleteID keyID = keys->getInitialID(source);
    internal->addIdOfInterest(keyID);
    // update liqui
    if (ch1->text() != nullptr) liqui=string(ch1->text());
    CompleteID liquiID = liquis->getID(liqui);
    internal->addIdOfInterest(liquiID);
    // add pair
    internal->addAccountPairOfInterest(source, liqui);

    checkThreadRunning=true;
    if (pthread_create(&checkDataThread, NULL, checkDataRoutine, (void*) this) < 0)
    {
        fl_alert("Failed to start thread.");
        checkThreadRunning=false;
        hide();
        return;
    }
    pthread_detach(checkDataThread);
}

TransferLiquiW::TransferLiquiW(InternalThread *i, string fixedLiqui, double amount, CompleteID targetT10EId)
    : Fl_Window(430, 180, "Transfer liquidity"), internal(i), targetId(targetT10EId), fixedAmount(amount), log(nullptr)
{
    Util u;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    source="";
    liqui="";
    target="";

    // choose source account
    box0 = new Fl_Box(20,10,166,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Source account:");
    ch0 = new Fl_Choice(190,10,220,25);
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
    box1 = new Fl_Box(20,40,166,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation:");
    ch1 = new Fl_Choice(190,40,220,25);
    ch1->add(fixedLiqui.c_str());
    ch1->callback(onChoice, (void*)this);
    ch1->take_focus();
    ch1->value(0);

    // set target
    box2 = new Fl_Box(20,70,166,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Request Id:");
    inpR = new Fl_Input(190,70,220,25);
    inpR->textsize(10);
    inpR->value(targetId.to27Char().c_str());
    inpR->readonly(1);

    // set amount
    box3 = new Fl_Box(20,100,226,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to be transferred:");
    inp = new Fl_Input(280,100,130,25);
    inp->value(u.toString(fixedAmount, 2).c_str());
    inp->readonly(1);

    but2 = new Fl_Button(265,140,145,25,"Send from account");
    but2->callback(TransferLiquiW::onSend, (void*)this);

    callback(TransferLiquiW::onClose, (void*)this);
    end();

    but1=nullptr;
    but2->deactivate();
    dataStatusClear=false;

    // update source
    if (ch0->text() != nullptr) source=string(ch0->text());
    CompleteID keyID = keys->getInitialID(source);
    internal->addIdOfInterest(keyID);
    // update liqui
    if (ch1->text() != nullptr) liqui=string(ch1->text());
    CompleteID liquiID = liquis->getID(liqui);
    internal->addIdOfInterest(liquiID);
    // add pair
    internal->addAccountPairOfInterest(source, liqui);

    checkThreadRunning=true;
    if (pthread_create(&checkDataThread, NULL, checkDataRoutine, (void*) this) < 0)
    {
        checkThreadRunning=false;
        return;
    }
    pthread_detach(checkDataThread);
}

TransferLiquiW::~TransferLiquiW()
{
    //dtor
}

void TransferLiquiW::onSend(Fl_Widget *w, void *d)
{
    Util u;
    TransferLiquiW *win = (TransferLiquiW*)d;

    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr
            || (win->targetId.isZero() && win->ch2->text()==nullptr))
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
    // update liqui
    str = string(win->ch1->text());
    if (str.compare(win->liqui) != 0)
    {
        win->liqui=str;
        CompleteID liquiID = win->liquis->getID(win->liqui);
        win->internal->addIdOfInterest(liquiID);
    }
    // add pair
    win->internal->addAccountPairOfInterest(win->source, win->liqui);
    // update target
    if (win->targetId.isZero())
    {
        str = string(win->ch2->text());
        if (str.compare(win->target) != 0)
        {
            win->target=str;
        }
    }

    // get amount
    double amount=0;
    if (win->targetId.isZero())
    {
        str = string(win->inp->value());
        if (str.length()<=0)
        {
            fl_alert("Error: %s", "no amount supplied.");
            return;
        }
        try
        {
            amount = stod(str);
        }
        catch(invalid_argument& e)
        {
            fl_alert("Error: %s", "bad amount supplied.");
            return;
        }
    }
    else amount=win->fixedAmount;
    if (amount<=0 || amount!=amount || isinf(amount))
    {
        fl_alert("Error: %s", "bad amount supplied.");
        return;
    }

    // get liquidity
    Type5Or15Entry *t5Or15e =  win->liquis->getLiqui(win->liqui);
    if (t5Or15e==nullptr)
    {
        fl_alert("Error: %s", "liquidity parameters unavailable.");
        return;
    }
    if (amount < t5Or15e->getMinTransferAmount())
    {
        fl_alert("Error: %s %s.", "minimal transfer amount is ", u.toString(t5Or15e->getMinTransferAmount(),2).c_str());
        return;
    }

    // build request
    string rqst;
    bool result = false;
    Fl_Button *but = (Fl_Button*)w;
    unsigned long notaryNr = win->connection->getNotaryNr();
    if (but == win->but2)
    {
        if (win->target.compare(win->source)==0)
        {
            fl_alert("Error: %s", "source and target cannot be the same for this operation.");
            return;
        }
        // check that win->source has sufficient funds in win->liqui
        Type14Entry *t14e =  win->requestBuilder->getLatestT14Ecopy(win->source, win->liqui);
        if (t14e==nullptr) result = false;
        else
        {
            if (t14e->getTransferableAmount()<amount)
            {
                fl_alert("Error: %s", "insufficient liquidity.");
                delete t14e;
                return;
            }
            // build request
            if (win->targetId.isZero())
            {
                result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, win->target, notaryNr, rqst);
                if (result)
                {
                    int answer = fl_choice("Are you sure you want to transfer %s units of liquidity %s\nfrom account %s to account %s?",
                                           "Close", "No", "Yes", u.toString(amount, 2).c_str(), win->liqui.c_str(),
                                           win->source.c_str(), win->target.c_str());
                    if (answer != 2)
                    {
                        delete t14e;
                        return;
                    }
                    // rebuild request
                    rqst="";
                    result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, win->target, notaryNr, rqst);
                }
            }
            else
            {
                result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, win->targetId, notaryNr, rqst);
                if (result)
                {
                    int answer = fl_choice("Are you sure you want to transfer %s units of liquidity %s\nfrom account %s to satisfy request %s?",
                                           "Close", "No", "Yes", u.toString(amount, 2).c_str(), win->liqui.c_str(),
                                           win->source.c_str(), win->targetId.to27Char().c_str());
                    if (answer != 2)
                    {
                        delete t14e;
                        return;
                    }
                    // rebuild request
                    rqst="";
                    result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, win->targetId, notaryNr, rqst);
                }
            }
            delete t14e;
        }
    }
    else if (but == win->but1)
    {
        // check that win->source is the owner of win->liqui
        if (t5Or15e==nullptr) result = false;
        else
        {
            if (t5Or15e->getType()!=15)
            {
                fl_alert("Error: %s", "this is possible for obligations only.");
                return;
            }
            CompleteID ownerId = ((Type15Entry*) t5Or15e)->getOwnerID();
            if (win->keys->getNameByID(ownerId).compare(win->source)!=0)
            {
                fl_alert("Error: %s", "incorrect obligation owner.");
                return;
            }
            // build request
            if (win->targetId.isZero())
            {
                if (win->source.compare(win->target)!=0)
                {
                    fl_alert("Error: %s", "source and target must coincide for this operation.");
                    return;
                }
                result = win->requestBuilder->createPrintAndTransferRqst(win->liqui, amount, win->source, win->target, notaryNr, rqst);
                if (result)
                {
                    int answer = fl_choice("Are you sure you want to create and transfer %s\nunits of obligation %s to account %s?",
                                           "Close", "No", "Yes", u.toString(amount, 2).c_str(), win->liqui.c_str(), win->target.c_str());
                    if (answer != 2) return;

                    // rebuild request
                    rqst="";
                    result = win->requestBuilder->createPrintAndTransferRqst(win->liqui, amount, win->source, win->target, notaryNr, rqst);
                }
            }
            else
            {
                fl_alert("Error: %s", "not permitted.");
                return;
            }
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

void TransferLiquiW::onChoice(Fl_Widget *w, void *d)
{
    TransferLiquiW *win = (TransferLiquiW*)d;

    if (!win->connection->connectionEstablished())
    {
        if (win->but1!=nullptr) win->but1->deactivate();
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
        win->internal->addAccountPairOfInterest(win->source, win->liqui);
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
        if (str.compare(win->source) != 0)
        {
            win->source=str;
            CompleteID keyID = win->keys->getInitialID(win->source);
            win->internal->addIdOfInterest(keyID);
        }
        win->internal->addAccountPairOfInterest(win->source, win->liqui);
    }
    else if (ch == win->ch2 && str.compare(win->target) != 0)
    {
        win->target=str;
        return;
    }
    if (win->but1!=nullptr) win->but1->deactivate();
    win->but2->deactivate();
    win->dataStatusClear=false;
}

void* TransferLiquiW::checkDataRoutine(void *w)
{
    TransferLiquiW* win=(TransferLiquiW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liqui);
        if (t5Or15e!=nullptr && t5Or15e->getType()==15)
        {
            CompleteID ownerId = ((Type15Entry*) t5Or15e)->getOwnerID();
            if (win->keys->getNameByID(ownerId).compare(win->source)==0)
            {
                if (win->but1!=nullptr) win->but1->activate();
            }
        }

        if (t5Or15e!=nullptr)
        {
            Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liqui);
            if (t14e!=nullptr)
            {
                win->dataStatusClear=true;
                win->but2->activate();
                delete t14e;
            }
            else win->dataStatusClear = false;
        }
        else win->dataStatusClear = false;

        usleep(500000);
    }
    return NULL;
}

void TransferLiquiW::onClose(Fl_Widget *w, void *d)
{
    TransferLiquiW *win = (TransferLiquiW*)d;
    win->checkThreadRunning=false;
    win->hide();
}

void TransferLiquiW::logInfo(const char *msg)
{
    if (log!=nullptr) log->info(msg);
}

void TransferLiquiW::logError(const char *msg)
{
    if (log!=nullptr) log->error(msg);
}

void TransferLiquiW::setLogger(Logger *l)
{
    log=l;
}
