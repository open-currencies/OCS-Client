#include "NewExchangeOfferW.h"

NewExchangeOfferW::NewExchangeOfferW(InternalThread *i) : Fl_Window(500, 210, "Request liquidity exchange"), internal(i)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();

    source="";
    liquiO="";
    liquiR="";

    // choose account
    box0 = new Fl_Box(20,10,236,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Account:");
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

    // choose currency
    box1 = new Fl_Box(20,40,236,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box1->label("Currency/Obligation offered:");
    ch1 = new Fl_Choice(260,40,220,25);
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
    box3 = new Fl_Box(20,70,324,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to be offered:");
    inp1 = new Fl_Input(350,70,130,25);

    // choose currency
    box2 = new Fl_Box(20,100,236,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Currency/Obligation requested:");
    ch2 = new Fl_Choice(260,100,220,25);
    for (it1=liquisNames.begin(); it1!=liquisNames.end(); ++it1)
    {
        ch2->add(it1->c_str());
    }
    ch2->callback(onChoice, (void*)this);
    ch2->take_focus();
    ch2->value(0);

    // set amount
    box4 = new Fl_Box(20,130,324,25);
    box4->labelfont(FL_COURIER);
    box4->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box4->label("Amount to be requested:");
    inp2 = new Fl_Input(350,130,130,25);

    but1 = new Fl_Button(178,170,145,25,"Place request");
    but1->callback(onSend, (void*)this);

    callback(NewExchangeOfferW::onClose, (void*)this);
    end();

    but1->deactivate();
    dataStatusClear=false;

    // update source
    if (ch0->text() != nullptr) source=string(ch0->text());
    CompleteID keyID = keys->getInitialID(source);
    internal->addIdOfInterest(keyID);
    // update liquiO
    if (ch1->text() != nullptr) liquiO=string(ch1->text());
    CompleteID liquiID = liquis->getID(liquiO);
    internal->addIdOfInterest(liquiID);
    // add pair
    internal->addAccountPairOfInterest(source, liquiO);
    // update liquiR
    if (ch2->text() != nullptr) liquiR=string(ch2->text());
    liquiID = liquis->getID(liquiR);
    internal->addIdOfInterest(liquiID);

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

NewExchangeOfferW::~NewExchangeOfferW()
{
    //dtor
}

void NewExchangeOfferW::onSend(Fl_Widget *w, void *d)
{
    NewExchangeOfferW *win = (NewExchangeOfferW*)d;

    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->ch0->text()==nullptr || win->ch1->text()==nullptr || win->ch2->text()==nullptr)
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
    // update liquiO
    str = string(win->ch1->text());
    if (str.compare(win->liquiO) != 0)
    {
        win->liquiO=str;
        CompleteID liquiID = win->liquis->getID(win->liquiO);
        win->internal->addIdOfInterest(liquiID);
    }
    // add pair
    win->internal->addAccountPairOfInterest(win->source, win->liquiO);
    // update liquiR
    str = string(win->ch2->text());
    if (str.compare(win->liquiR) != 0)
    {
        win->liquiR=str;
        CompleteID liquiID = win->liquis->getID(win->liquiR);
        win->internal->addIdOfInterest(liquiID);
    }
    if (win->liquiR.compare(win->liquiO) == 0)
    {
        fl_alert("Error: %s", "offered and requested liquidities must differ.");
        return;
    }
    // get amount offered
    str = string(win->inp1->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no offered amount supplied.");
        return;
    }
    double amountO=0;
    try
    {
        amountO = stod(str);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad offered amount supplied.");
        return;
    }
    if (amountO<=0 || amountO!=amountO || isinf(amountO))
    {
        fl_alert("Error: %s", "bad offered amount supplied.");
        return;
    }
    Util u;
    // get liquidity
    Type5Or15Entry *t5Or15eO =  win->liquis->getLiqui(win->liquiO);
    if (t5Or15eO==nullptr)
    {
        fl_alert("Error: %s", "liquidity (offered) parameters unavailable.");
        return;
    }
    if (amountO < t5Or15eO->getMinTransferAmount())
    {
        fl_alert("Error: %s %s.", "minimal transfer amount (offered liquidity) is ", u.toString(t5Or15eO->getMinTransferAmount(),2).c_str());
        return;
    }
    // get amount requested
    str = string(win->inp2->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no requested amount supplied.");
        return;
    }
    double amountR=0;
    try
    {
        amountR = stod(str);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad requested amount supplied.");
        return;
    }
    if (amountR<=0 || amountR!=amountR || isinf(amountR))
    {
        fl_alert("Error: %s", "bad requested amount supplied.");
        return;
    }
    // get liquidity
    Type5Or15Entry *t5Or15eR =  win->liquis->getLiqui(win->liquiR);
    if (t5Or15eR==nullptr)
    {
        fl_alert("Error: %s", "liquidity (offered) parameters unavailable.");
        return;
    }
    if (amountR < t5Or15eR->getMinTransferAmount())
    {
        fl_alert("Error: %s %s.", "minimal transfer amount (requested liquidity) is ", u.toString(t5Or15eR->getMinTransferAmount(),2).c_str());
        return;
    }
    // build request
    string rqst;
    bool result = false;
    unsigned long notaryNr = win->connection->getNotaryNr();
    Type14Entry *t14e =  win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiO);
    if (t14e==nullptr) result = false;
    else
    {
        // check that account has sufficient funds
        if (t14e->getTransferableAmount()<amountO)
        {
            fl_alert("Error: %s", "insufficient liquidity on account.");
            delete t14e;
            return;
        }
        int answer = fl_choice("Are you sure you want to place an exchange offer for %s units\nof liquidity %s for the source account %s\nrequesting %s units of liquidity %s?",
                               "Close", "No", "Yes", u.toString(amountO, 2).c_str(), win->liquiO.c_str(), win->source.c_str(), u.toString(amountR, 2).c_str(), win->liquiR.c_str());
        if (answer != 2)
        {
            delete t14e;
            return;
        }
        // build request
        result = win->requestBuilder->createExchangeOfferRqst(win->liquiO, amountO, win->liquiR, amountR, win->source, notaryNr, rqst);
        delete t14e;
    }
    // send request
    if (result)
    {
        // rebuild request
        rqst="";
        result = win->requestBuilder->createExchangeOfferRqst(win->liquiO, amountO, win->liquiR, amountR, win->source, notaryNr, rqst);
        if (!result)
        {
            fl_alert("Error: %s", "insufficient data to process request.");
            return;
        }
        win->connection->sendRequest(rqst);
        fl_alert("Info: %s", "an exchange offer was sent for notarization.\nPlease verify its placement and retrieve its Id under \"Existing offers\".");
        win->hide();
    }
    else
    {
        fl_alert("Error: %s", "insufficient data to process request.");
    }
}

void NewExchangeOfferW::onChoice(Fl_Widget *w, void *d)
{
    NewExchangeOfferW *win = (NewExchangeOfferW*)d;

    if (!win->connection->connectionEstablished())
    {
        win->but1->deactivate();
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
        str = string(win->ch1->text());
        // update liquiO
        if (str.compare(win->liquiO) != 0)
        {
            win->liquiO=str;
            CompleteID liquiID = win->liquis->getID(win->liquiO);
            win->internal->addIdOfInterest(liquiID);
        }
        // add pair
        win->internal->addAccountPairOfInterest(win->source, win->liquiO);
    }
    else if (ch == win->ch1 && str.compare(win->liquiO) != 0)
    {
        // update liquiO
        win->liquiO=str;
        CompleteID liquiID = win->liquis->getID(win->liquiO);
        win->internal->addIdOfInterest(liquiID);
        // update key
        str = string(win->ch0->text());
        if (str.compare(win->source) != 0)
        {
            win->source=str;
            CompleteID keyID = win->keys->getInitialID(win->source);
            win->internal->addIdOfInterest(keyID);
        }
        win->internal->addAccountPairOfInterest(win->source, win->liquiO);
    }
    else if (ch == win->ch2 && str.compare(win->liquiR) != 0)
    {
        // update liquiR
        win->liquiR=str;
        CompleteID liquiID = win->liquis->getID(win->liquiR);
        win->internal->addIdOfInterest(liquiID);
    }
    win->but1->deactivate();
    win->dataStatusClear=false;
}

void NewExchangeOfferW::onClose(Fl_Widget *w, void *d)
{
    NewExchangeOfferW *win = (NewExchangeOfferW*)d;
    win->checkThreadRunning=false;
    win->hide();
}

void* NewExchangeOfferW::checkDataRoutine(void *w)
{
    NewExchangeOfferW* win=(NewExchangeOfferW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        Type5Or15Entry *t5Or15eO = win->liquis->getLiqui(win->liquiO);
        Type5Or15Entry *t5Or15eR = win->liquis->getLiqui(win->liquiR);
        if (t5Or15eO!=nullptr && t5Or15eR!=nullptr)
        {
            Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->source, win->liquiO);
            if (t14e!=nullptr)
            {
                win->dataStatusClear = true;
                win->but1->activate();
                delete t14e;
            }
            else win->dataStatusClear = false;
        }
        else win->dataStatusClear = false;

        usleep(500000);
    }
    pthread_exit(NULL);
}
