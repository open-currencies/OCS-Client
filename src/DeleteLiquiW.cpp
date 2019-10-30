#include "DeleteLiquiW.h"

DeleteLiquiW::DeleteLiquiW(InternalThread *i) : Fl_Window(430, 150, "Delete liquidity"), internal(i)
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

    // set amount
    box3 = new Fl_Box(20,70,226,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to be deleted:");
    inp = new Fl_Input(280,70,130,25);

    but = new Fl_Button(265,110,155,25,"Delete from account");
    but->callback(DeleteLiquiW::onSend, (void*)this);

    callback(DeleteLiquiW::onClose, (void*)this);
    end();

    but->deactivate();
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

DeleteLiquiW::~DeleteLiquiW()
{
    //dtor
}

void DeleteLiquiW::onSend(Fl_Widget *w, void *d)
{
    Util u;
    DeleteLiquiW *win = (DeleteLiquiW*)d;

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

    // get amount
    double amount=0;
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
        fl_alert("Error: %s %s.", "minimal amount is ", u.toString(t5Or15e->getMinTransferAmount(),2).c_str());
        return;
    }
    if (t5Or15e->getType()==15)
    {
        CompleteID ownerId = ((Type15Entry*) t5Or15e)->getOwnerID();
        if (win->keys->getNameByID(ownerId).compare(win->source)!=0)
        {
            fl_alert("Error: %s", "only obligation owner is permitted to delete.");
            return;
        }
    }

    // build request
    string rqst;
    bool result = false;
    unsigned long notaryNr = win->connection->getNotaryNr();
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
        CompleteID zeroId;
        result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, zeroId, notaryNr, rqst);
        if (result)
        {
            int answer = fl_choice("Are you sure you want to delete %s units of liquidity %s\nfrom account %s?",
                                   "Close", "No", "Yes", u.toString(amount, 2).c_str(), win->liqui.c_str(),
                                   win->source.c_str());
            if (answer != 2)
            {
                delete t14e;
                return;
            }
            // rebuild request
            rqst="";
            result = win->requestBuilder->createTransferRqst(win->liqui, amount, win->source, zeroId, notaryNr, rqst);
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

void DeleteLiquiW::onChoice(Fl_Widget *w, void *d)
{
    DeleteLiquiW *win = (DeleteLiquiW*)d;

    if (!win->connection->connectionEstablished())
    {
        win->but->deactivate();
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
        if (win->ch1->text()!=nullptr) str = string(win->ch1->text());
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
        if (win->ch0->text()!=nullptr) str = string(win->ch0->text());
        else str="";
        if (str.compare(win->source) != 0)
        {
            win->source=str;
            CompleteID keyID = win->keys->getInitialID(win->source);
            win->internal->addIdOfInterest(keyID);
        }
        win->internal->addAccountPairOfInterest(win->source, win->liqui);
    }
    win->but->deactivate();
    win->dataStatusClear=false;
}

void* DeleteLiquiW::checkDataRoutine(void *w)
{
    DeleteLiquiW* win=(DeleteLiquiW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        Type5Or15Entry *t5Or15e = win->liquis->getLiqui(win->liqui);
        if (t5Or15e!=nullptr)
        {
            Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liqui);
            if (t14e!=nullptr)
            {
                win->dataStatusClear=true;
                win->but->activate();
                delete t14e;
            }
            else win->dataStatusClear = false;
        }
        else win->dataStatusClear = false;

        usleep(500000);
    }
    return NULL;
}

void DeleteLiquiW::onClose(Fl_Widget *w, void *d)
{
    DeleteLiquiW *win = (DeleteLiquiW*)d;
    win->checkThreadRunning=false;
    win->hide();
}
