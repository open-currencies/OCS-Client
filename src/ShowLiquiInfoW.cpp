#include "ShowLiquiInfoW.h"

InternalThread* ShowLiquiInfoW::internal;
KeysHandling* ShowLiquiInfoW::keys;
LiquiditiesHandling* ShowLiquiInfoW::liquis;
list<Fl_Window*>* ShowLiquiInfoW::subWins;
mutex ShowLiquiInfoW::winByLiquiIdMutex;
map<string, ShowLiquiInfoW*> ShowLiquiInfoW::winByLiquilId;

ShowLiquiInfoW::ShowLiquiInfoW(list<Fl_Window*> *s, InternalThread *i, bool f) : Fl_Window(580, 480, "Show info of currency/obligation"),
    forRefs(f), type5Or15entryC(nullptr), waitForDataThread(nullptr)
{
    internal=i;
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    if (forRefs)
    {
        copy_label("Show info of currency");
    }

    // choose name
    box0 = new Fl_Box(20,15,166,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box0->label("Name (optional):");
    ch0 = new Fl_Choice(190,15,230,25);
    ch0->add(" ");
    list<string> liquisNames;
    liquis->loadKnownLiquidities(liquisNames);
    list<string>::iterator it;
    for (it=liquisNames.begin(); it!=liquisNames.end(); ++it)
    {
        string liquiName = *it;
        Type5Or15Entry* t5Or15e = liquis->getLiqui(liquiName);
        if (!forRefs || t5Or15e == nullptr || t5Or15e->getType() == 5)
        {
            ch0->add(liquiName.c_str());
        }
    }
    ch0->take_focus();
    ch0->value(0);

    // Liquidity id
    box = new Fl_Box(20,45,166,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box->label("ID (optional):");
    inp = new Fl_Input(190,45,230,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);

    // request button
    but = new Fl_Button(450,45,110,25,"Request data");
    but->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 85, 540, 375, 5);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");
    b->link(linkRoutine);

    end();
    resizable(b);
}

ShowLiquiInfoW::~ShowLiquiInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
    if (type5Or15entryC!=nullptr) delete type5Or15entryC;
}

bool ShowLiquiInfoW::printOutInfoOnLiqui(CompleteID &firstId, Type5Or15Entry* t5Or15e, ShowLiquiInfoW *win)
{
    if (t5Or15e == nullptr) return false;
    string htmlTxt;
    string firstIdStr = firstId.to27Char();
    Util u;
    // report type
    if (t5Or15e->getType() == 5)
    {
        htmlTxt.append("<b>Type: currency </b>");
        if (!win->forRefs)
        {
            htmlTxt.append(" &nbsp;&nbsp;&nbsp;&nbsp;<a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(firstIdStr);
            htmlTxt.push_back('L');
            htmlTxt.append("\">APPLY FOR LIQUIDITY</a>");
        }
        else
        {
            htmlTxt.append(" &nbsp;&nbsp;&nbsp;&nbsp;<a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(firstIdStr);
            htmlTxt.push_back('R');
            htmlTxt.append("\">APPLY FOR REFEREE STATUS</a>");
        }
    }
    else htmlTxt.append("<b>Type: obligation</b>");
    htmlTxt.append("<br>\n&nbsp;<br>\n");
    // report time
    htmlTxt.append("Registration time: ");
    htmlTxt.append(u.epochToStr(firstId.getTimeStamp()));
    htmlTxt.append("<br>\n");
    // report id
    if (t5Or15e->getType() == 5) htmlTxt.append("Currency ");
    else htmlTxt.append("Obligation ");
    htmlTxt.append("Id: <font face=\"courier\" size=\"2\">");
    htmlTxt.append(firstIdStr);
    string liquiName = win->liquis->getNameById(firstId);
    if (liquiName.length()==0)
    {
        htmlTxt.append(" </font> <a style=\"text-decoration: none\" href=\"");
        htmlTxt.append(firstIdStr);
        htmlTxt.push_back('N');
        htmlTxt.append("\">ASSIGN NAME</a><br>\n");
    }
    else
    {
        htmlTxt.append("</font> (");
        htmlTxt.append(liquiName);
        htmlTxt.append(")<br>\n");
    }
    // type specific info
    if (t5Or15e->getType() == 15)
    {
        Type15Entry* t15e = (Type15Entry*) t5Or15e;
        CompleteID ownerID = t15e->getOwnerID();
        // print owner
        htmlTxt.append("Owner Id: <font face=\"courier\" size=\"2\">");
        htmlTxt.append(ownerID.to27Char());
        string ownerName = win->keys->getNameByID(ownerID);
        if (ownerName.length()==0)
        {
            htmlTxt.append(" </font> <a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(ownerID.to27Char());
            htmlTxt.push_back('O');
            htmlTxt.append("\">ASSIGN NAME</a><br>\n");
        }
        else
        {
            htmlTxt.append("</font> (");
            htmlTxt.append(ownerName);
            htmlTxt.append(")<br>\n");
        }
        // print rate
        if (t15e->getRate()<0)
        {
            htmlTxt.append("Discount rate (in % per year): ");
            htmlTxt.append(u.toString(-t15e->getRate(), 2));
        }
        else
        {
            htmlTxt.append("Growth rate (in % per year): ");
            htmlTxt.append(u.toString(t15e->getRate(), 2));
        }
        htmlTxt.append("<br>\n");
        // minTransferAmount
        htmlTxt.append("minimal liquidity transfer amount: ");
        htmlTxt.append(u.toString(t15e->getMinTransferAmount(), 2));
        htmlTxt.append("<br>\n");
        // print description
        htmlTxt.append("<i>Description:</i><br><hr>\n");
        string prefix("<!doctype creole>");
        if (t15e->getDescription()->compare(0, prefix.length(), prefix) == 0)
        {
            size_t descriptionLength = t15e->getDescription()->length() - prefix.length();
            string description = t15e->getDescription()->substr(prefix.length(), descriptionLength);
            NME nme(description.c_str(), descriptionLength);
            nme.setFormat(NMEOutputFormatHTML);
            NMEConstText output;
            if (nme.getOutput(&output) == kNMEErrOk)
            {
                htmlTxt.append((char*) output);
            }
            else htmlTxt.append("Error while parsing Creole.");
        }
        else htmlTxt.append(*t15e->getDescription());
    }
    else
    {
        Type5Entry* t5e = (Type5Entry*) t5Or15e;
        if (win->forRefs)
        {
            CompleteID ownerID = t5e->getOwnerID();
            // print owner
            htmlTxt.append("First referee Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(ownerID.to27Char());
            string ownerName = win->keys->getNameByID(ownerID);
            if (ownerName.length()==0)
            {
                htmlTxt.append(" </font> <a style=\"text-decoration: none\" href=\"");
                htmlTxt.append(ownerID.to27Char());
                htmlTxt.push_back('O');
                htmlTxt.append("\">ASSIGN NAME</a><br>\n");
            }
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(ownerName);
                htmlTxt.append(")<br>\n");
            }
        }
        if (!win->forRefs)
        {
            // minProcessingTime
            htmlTxt.append("min. operation proposal processing time (days): ");
            htmlTxt.append(u.toString(1.0/60/60/24*t5e->minOpPrProcessingTime(), 2));
            htmlTxt.append("<br>\n");
            // maxProcessingTime
            htmlTxt.append("max. operation proposal processing time (days): ");
            htmlTxt.append(u.toString(1.0/60/60/24*t5e->maxOpPrProcessingTime(), 2));
            htmlTxt.append("<br>\n");
        }
        else
        {
            // processingTime
            htmlTxt.append("referee application processing time (days): ");
            htmlTxt.append(u.toString(1.0/60/60/24*t5e->getRefApplProcessingTime(), 2));
            htmlTxt.append("<br>\n");
        }
        if (!win->forRefs)
        {
            // processing fee range for op proposals
            htmlTxt.append("operation proposal processing fee: ");
            htmlTxt.append("<a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(firstIdStr);
            htmlTxt.push_back('F');
            htmlTxt.append("\">CALCULATE RANGE</a><br>\n");
        }
        else
        {
            // stake for op proposals
            htmlTxt.append("stake for operation proposals: ");
            htmlTxt.append("<a style=\"text-decoration: none\" href=\"");
            htmlTxt.append(firstIdStr);
            htmlTxt.push_back('S');
            htmlTxt.append("\">CALCULATE</a><br>\n");

            // referee application stake
            htmlTxt.append("referee application stake: ");
            htmlTxt.append(u.toString(t5e->getRefApplStake(), 2));
            htmlTxt.append("<br>\n");
            // max. liquidity approval
            htmlTxt.append("max. liquidity approval amount per referee: ");
            htmlTxt.append(u.toString(t5e->getLiquidityCreationLimit(), 2));
            htmlTxt.append("<br>\n");
            // referee application stake
            htmlTxt.append("max. referee appointments per referee: ");
            htmlTxt.append(to_string(t5e->getRefAppointmentLimit()));
            htmlTxt.append("<br>\n");
            // processingTime
            htmlTxt.append("referee tenure duration (days): ");
            htmlTxt.append(u.toString(1.0/60/60/24*t5e->getRefereeTenure(), 2));
            htmlTxt.append("<br>\n");
            // initial liquidity
            htmlTxt.append("initial liquidity per appointed referee: ");
            htmlTxt.append(u.toString(t5e->getInitialLiquidity(), 2));
            htmlTxt.append("<br>\n");
            // parent share
            htmlTxt.append("profit share with appointing referee in %: ");
            htmlTxt.append(u.toString(t5e->getParentShare(), 2));
            htmlTxt.append("<br>\n");
        }
        if (!win->forRefs)
        {
            // print rate
            if (t5e->getRate()<0)
            {
                htmlTxt.append("Discount rate (in % per year): ");
                htmlTxt.append(u.toString(-t5e->getRate(), 2));
            }
            else
            {
                htmlTxt.append("Growth rate (in % per year): ");
                htmlTxt.append(u.toString(t5e->getRate(), 2));
            }
            htmlTxt.append("<br>\n");
            // minTransferAmount
            htmlTxt.append("minimal liquidity transfer amount: ");
            htmlTxt.append(u.toString(t5e->getMinTransferAmount(), 2));
            htmlTxt.append("<br>\n");
        }
        // print description
        htmlTxt.append("<i>Description:</i><br><hr>\n");
        string prefix("<!doctype creole>");
        if (t5e->getDescription()->compare(0, prefix.length(), prefix) == 0)
        {
            size_t descriptionLength = t5e->getDescription()->length() - prefix.length();
            string description = t5e->getDescription()->substr(prefix.length(), descriptionLength);
            NME nme(description.c_str(), descriptionLength);
            nme.setFormat(NMEOutputFormatHTML);
            NMEConstText output;
            if (nme.getOutput(&output) == kNMEErrOk)
            {
                htmlTxt.append((char*) output);
            }
            else htmlTxt.append("Error while parsing Creole.");
        }
        else htmlTxt.append(*t5e->getDescription());
    }

    // set identifier
    winByLiquiIdMutex.lock();
    if (winByLiquilId.count(firstIdStr)<1)
    {
        winByLiquilId.insert(pair<string, ShowLiquiInfoW*>(firstIdStr, win));
    }
    else winByLiquilId[firstIdStr]=win;
    if (win->type5Or15entryC!=nullptr)
    {
        delete win->type5Or15entryC;
        win->type5Or15entryC=nullptr;
    }
    win->type5Or15entryC = (Type5Or15Entry*) t5Or15e->createCopy();
    winByLiquiIdMutex.unlock();

    // print out
    win->b->value(htmlTxt.c_str());
    win->b->redraw();

    return true;
}

void ShowLiquiInfoW::onRequest(Fl_Widget *w, void *d)
{
    ShowLiquiInfoW *win = (ShowLiquiInfoW*)d;
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
    if (win->ch0->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // get liqui id
    CompleteID liquiId;
    string str = string(win->ch0->text());
    string strInp(win->inp->value());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), ' '), strInp.end());
    strInp.erase(std::remove(strInp.begin(), strInp.end(), '-'), strInp.end());
    if (str.length()>0 && str.compare(" ")!=0)
    {
        if (strInp.length()>0)
        {
            fl_alert("Error: %s", "please choose between Liquidity name and Liquidity ID\nby deleting either one.");
            return;
        }
        liquiId = win->liquis->getID(str);
    }
    else
    {
        // get liquiId id from input field
        if (strInp.length()<=0)
        {
            fl_alert("Error: %s", "no Liquidity ID supplied.");
            return;
        }
        if (strInp.length()>0 && strInp.length()!=27)
        {
            fl_alert("Error: %s", "supplied Liquidity ID does not have an admissible length.");
            return;
        }
        liquiId = CompleteID(strInp);
        if (liquiId.getNotary()<=0)
        {
            fl_alert("Error: %s", "supplied Liquidity ID does not have an admissible format.");
            return;
        }
    }

    // print out immediately if already stored
    string name = win->liquis->getNameById(liquiId);
    Type5Or15Entry* entry = win->liquis->getLiqui(name);
    if (entry != nullptr)
    {
        printOutInfoOnLiqui(liquiId, entry, win);
        return;
    }

    // build and send request
    string rqst;
    bool result = win->requestBuilder->createIdInfoRqst(liquiId, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForDataThread, NULL, ShowLiquiInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowLiquiInfoW::waitForDataRoutine(void *w)
{
    ShowLiquiInfoW* win=(ShowLiquiInfoW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->b->value("<h3>Loading ...</h3>");
    win->b->redraw();
    list<pair<Type12Entry*, CIDsSet*>>* entriesList = nullptr;
    int rounds=0;
    do
    {
        usleep(300000);
        entriesList = win->msgProcessor->getProcessedRqst(requestNumber);
        rounds++;
    }
    while (entriesList == nullptr && rounds < 20 && win->internal->running);
    // display entries list
    if (entriesList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (entriesList->size()<=0 || (entriesList->begin()->first->underlyingType()!=5
                                        && entriesList->begin()->first->underlyingType()!=15) )
    {
        win->b->value("<h3>No such liquidity entry reported.</h3>");
        win->b->redraw();
    }
    else if (win->forRefs && entriesList->begin()->first->underlyingType()!=5)
    {
        win->b->value("<h3>Is not a currency.</h3>");
        win->b->redraw();
    }
    else
    {
        // get id and related stuff
        list<pair<Type12Entry*, CIDsSet*>>::iterator it=entriesList->begin();
        Type12Entry* t12e = it->first;
        Type5Or15Entry *t5Or15e = (Type5Or15Entry*) t12e->underlyingEntry();
        CompleteID firstId = it->second->first();
        // print out
        printOutInfoOnLiqui(firstId, t5Or15e, win);
    }
    usleep(200000);
    // clean up and exit
    Fl::flush();
    win->msgProcessor->deleteOldRqst(requestNumber);
    win->rqstNum = 0;
    return NULL;
}

const char* ShowLiquiInfoW::linkRoutine(Fl_Widget *w, const char *uri)
{
    size_t len = strlen(uri);
    char linkRequestType = uri[len-1];
    string str(uri, len-1);
    CompleteID id(str);
    if (id.getNotary()<=0)
    {
        fl_alert("Error: %s", "bad id.");
        return NULL;
    }

    if (linkRequestType == 'N')
    {
        // assign liqui name
        AssignLiquiNameW *assignLiquiNameW = new AssignLiquiNameW(liquis, id);
        subWins->insert(subWins->end(), (Fl_Window*)assignLiquiNameW);
        ((Fl_Window*)assignLiquiNameW)->show();
    }
    else if (linkRequestType == 'L')
    {
        // create OperationProposalW
        string liqui = liquis->getNameById(id);
        if (liqui.length()>0)
        {
            OperationProposalW *operationProposalW = new OperationProposalW(subWins, internal, liqui);
            subWins->insert(subWins->end(), (Fl_Window*)operationProposalW);
            ((Fl_Window*)operationProposalW)->show();
        }
    }
    else if (linkRequestType == 'R')
    {
        // create RefereeApplicationW
        string liqui = liquis->getNameById(id);
        if (liqui.length()>0)
        {
            RefereeApplicationW *refereeApplicationW = new RefereeApplicationW(subWins, internal, liqui);
            subWins->insert(subWins->end(), (Fl_Window*)refereeApplicationW);
            ((Fl_Window*)refereeApplicationW)->show();
        }
    }
    else if (linkRequestType == 'O')
    {
        // create NewAccountW
        NewAccountW *newAccountW = new NewAccountW(keys, id);
        subWins->insert(subWins->end(), (Fl_Window*)newAccountW);
        ((Fl_Window*)newAccountW)->show();
    }
    else if (linkRequestType == 'F')
    {
        winByLiquiIdMutex.lock();
        if (winByLiquilId.count(str)<1) return NULL;
        ShowLiquiInfoW* win=winByLiquilId[str];
        Type5Entry *t5e = (Type5Entry*) win->type5Or15entryC->createCopy();
        winByLiquiIdMutex.unlock();
        // create FeeAndStakeCalcW
        FeeAndStakeCalcW *feeAndStakeCalcW = new FeeAndStakeCalcW(t5e, id, internal);
        subWins->insert(subWins->end(), (Fl_Window*)feeAndStakeCalcW);
        ((Fl_Window*)feeAndStakeCalcW)->show();
    }
    else if (linkRequestType == 'S')
    {
        winByLiquiIdMutex.lock();
        if (winByLiquilId.count(str)<1) return NULL;
        ShowLiquiInfoW* win=winByLiquilId[str];
        Type5Entry *t5e = (Type5Entry*) win->type5Or15entryC->createCopy();
        winByLiquiIdMutex.unlock();
        // create FeeAndStakeCalcW
        FeeAndStakeCalcW *feeAndStakeCalcW = new FeeAndStakeCalcW(t5e, id, internal);
        subWins->insert(subWins->end(), (Fl_Window*)feeAndStakeCalcW);
        ((Fl_Window*)feeAndStakeCalcW)->show();
    }

    // exit
    return NULL;
}
