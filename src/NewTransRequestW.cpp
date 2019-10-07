#include "NewTransRequestW.h"

NewTransRequestW::NewTransRequestW(InternalThread *i) : Fl_Window(430, 150, "Request liquidity"), internal(i)
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
    box0->label("Target account:");
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
    box3 = new Fl_Box(20,70,230,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Amount to be requested:");
    inp = new Fl_Input(280,70,130,25);

    but1 = new Fl_Button(160,110,145,25,"Place request");
    but1->callback(onSend, (void*)this);

    end();

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
}

NewTransRequestW::~NewTransRequestW()
{
    //dtor
}

void NewTransRequestW::onSend(Fl_Widget *w, void *d)
{
    NewTransRequestW *win = (NewTransRequestW*)d;

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
    str = string(win->inp->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no amount supplied.");
        return;
    }
    double amount=0;
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
    // build request
    string rqst;
    bool result = false;
    Util u;
    unsigned long notaryNr = win->connection->getNotaryNr();

    // build request
    result = win->requestBuilder->createTransferRqstRqst(win->liqui, amount, win->source, notaryNr, rqst);
    // send request
    if (result)
    {
        int answer = fl_choice("Are you sure you want to place a transfer request for %s units\nof liquidity %s for the receiving account %s?",
                               "Close", "No", "Yes", u.toString(amount, 2).c_str(), win->liqui.c_str(), win->source.c_str());
        if (answer != 2) return;
        // rebuild request
        rqst="";
        result = win->requestBuilder->createTransferRqstRqst(win->liqui, amount, win->source, notaryNr, rqst);
        if (!result)
        {
            fl_alert("Error: %s", "insufficient data to process request.");
            return;
        }
        win->connection->sendRequest(rqst);
        fl_alert("Info: %s", "a transfer request was sent for notarization.\nPlease verify its placement and retrieve its Id under \"Existing requests\".");
        win->hide();
    }
    else
    {
        fl_alert("Error: %s", "insufficient data to process request.");
    }
}

void NewTransRequestW::onChoice(Fl_Widget *w, void *d)
{
    NewTransRequestW *win = (NewTransRequestW*)d;

    if (!win->connection->connectionEstablished())
    {
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
}
