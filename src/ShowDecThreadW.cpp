#include "ShowDecThreadW.h"

mutex ShowDecThreadW::winByApplIdMutex;
map<string, ShowDecThreadW*> ShowDecThreadW::winByApplId;

ShowDecThreadW::ShowDecThreadW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(640, 480, "Show decision thread"),
    subWins(s), internal(i), thread(nullptr), waitForDataThread(nullptr)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // thread id
    box = new Fl_Box(20,15,100,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->label("Thread ID:");
    inp = new Fl_Input(130,15,220,25);
    inp->textsize(10);
    inp->textfont(FL_COURIER);

    // request button
    but = new Fl_Button(510,15,110,25,"Request data");
    but->callback(onRequest, (void*)this);

    // create html area
    b = new Fl_Html_View(20, 55, 600, 405, 0);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");
    b->link(linkRoutine);

    end();
    resizable(b);
}

ShowDecThreadW::~ShowDecThreadW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
    msgProcessor->deleteList(thread);
}

void ShowDecThreadW::onRequest(Fl_Widget *w, void *d)
{
    ShowDecThreadW *win = (ShowDecThreadW*)d;
    if (win->inp->readonly() != 0 || win->but->visible() == 0 || win->thread != nullptr)
    {
        fl_alert("Error: %s", "thread already found.");
        return;
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
    // get Thread id
    string str(win->inp->value());
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '-'), str.end());
    if (str.length()>0 && str.length()!=27)
    {
        fl_alert("Error: %s", "supplied Thread ID does not have an admissible length.");
        return;
    }
    CompleteID threadId(str);
    if (threadId.getNotary()<=0)
    {
        fl_alert("Error: %s", "supplied Thread ID does not have an admissible format.");
        return;
    }
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createIdInfoRqst(threadId, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create(win->waitForDataThread, NULL, ShowDecThreadW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowDecThreadW::waitForDataRoutine(void *w)
{
    ShowDecThreadW* win=(ShowDecThreadW*) w;
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
    // display claims list
    bool error=true;
    if (entriesList==nullptr)
    {
        win->b->value("<h3>Request timeout. No data received.</h3>");
        win->b->redraw();
    }
    else if (entriesList->size()<=0 || (entriesList->begin()->first->underlyingType()!=6 && entriesList->begin()->first->underlyingType()!=7
                                        && entriesList->begin()->first->underlyingType()!=3))
    {
        win->b->value("<h3>No such thread reported.</h3>");
        win->b->redraw();
    }
    else
    {
        // thread received, store data
        win->thread=entriesList;
        win->but->hide();
        win->inp->readonly(1);

        Util u;

        // display thread data
        string htmlTxt;
        unsigned long long terminationTime = printOutThread(win->thread, win->keys, win->liquis, htmlTxt);
        if (terminationTime!=ULLONG_MAX)
        {
            error = false;
            size_t status = win->thread->size();
            string applIdStr = win->thread->begin()->second->first().to27Char();
            // build head
            htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\">");
            if (terminationTime!=0)
            {
                if (status % 2 == 1)
                {
                    htmlTxt.append("<i>The application is currently in rejected state.<br>The thread will terminate on ");
                    htmlTxt.append(u.epochToStr(terminationTime));
                    htmlTxt.append(" unless an approval is submitted.&nbsp;&nbsp;");
                    htmlTxt.append(" <a style=\"text-decoration: none\" href=\"");
                    htmlTxt.append(applIdStr);
                    htmlTxt.append("\">WRITE APPROVAL</a></i>");
                }
                else
                {
                    htmlTxt.append("<i>The application is currently in approved state.<br>The thread will terminate on ");
                    htmlTxt.append(u.epochToStr(terminationTime));
                    htmlTxt.append(" unless a rejection is submitted.&nbsp;&nbsp;");
                    htmlTxt.append(" <a style=\"text-decoration: none\" href=\"");
                    htmlTxt.append(applIdStr);
                    htmlTxt.append("\">WRITE REJECTION</a></i>");
                }
                // store data for the link
                winByApplIdMutex.lock();
                if (winByApplId.count(applIdStr)<1)
                {
                    winByApplId.insert(pair<string, ShowDecThreadW*>(applIdStr, win));
                }
                else winByApplId[applIdStr]=win;
                winByApplIdMutex.unlock();
            }
            else
            {
                CompleteID terminationId = win->thread->rbegin()->second->first();
                // report time
                htmlTxt.append("Thread termination time: ");
                htmlTxt.append(u.epochToStr(terminationId.getTimeStamp()));
                if (status % 2 == 0) htmlTxt.append(" (Rejected)");
                else
                {
                    htmlTxt.append(" (Approved)");
                    // add link for type 2 entry creation:
                    bool recentNotaryApproval = false;
                    if (win->thread->begin()->first->underlyingType() == 3) recentNotaryApproval = true;
                    // check that appointment not too old
                    unsigned long long currentTime = win->requestBuilder->currentSyncTimeInMs();
                    if (currentTime == 0) recentNotaryApproval = false;
                    if (recentNotaryApproval)
                    {
                        unsigned long long approvalTime = terminationId.getTimeStamp();
                        Type1Entry *t1e = win->keys->getType1Entry();
                        unsigned short lineage = t1e->getLineage(approvalTime);
                        if (lineage != t1e->getLineage(currentTime)) recentNotaryApproval = false;
                        else
                        {
                            unsigned long long recentUntil = (unsigned long long) t1e->getNotaryTenure(lineage);
                            recentUntil *= 1000;
                            recentUntil += approvalTime;
                            if (currentTime > recentUntil) recentNotaryApproval = false;
                        }
                    }
                    // check that lineage has not changed
                    if (recentNotaryApproval)
                    {
                        unsigned long long applicationTime = win->thread->begin()->second->first().getTimeStamp();
                        Type1Entry *t1e = win->keys->getType1Entry();
                        unsigned short lineage = t1e->getLineage(applicationTime);
                        if (lineage != t1e->getLineage(currentTime)) recentNotaryApproval = false;
                    }
                    // check that no notary status yet
                    if (recentNotaryApproval)
                    {
                        CompleteID applicantId = win->thread->begin()->first->pubKeyID();
                        string keyName = win->keys->getNameByID(applicantId);
                        string *pubKeyStr = win->keys->getPublicKeyStr(keyName);
                        if (pubKeyStr == nullptr || !win->keys->getNotary(pubKeyStr).isZero()) recentNotaryApproval = false;
                    }
                    // invite to become notary
                    if (recentNotaryApproval)
                    {
                        htmlTxt.append("&nbsp;&nbsp; <a style=\"text-decoration: none\" href=\"");
                        htmlTxt.append(applIdStr);
                        htmlTxt.append("\">BECOME NOTARY</a>");
                        // store data for the link
                        winByApplIdMutex.lock();
                        if (winByApplId.count(applIdStr)<1)
                        {
                            winByApplId.insert(pair<string, ShowDecThreadW*>(applIdStr, win));
                        }
                        else winByApplId[applIdStr]=win;
                        winByApplIdMutex.unlock();
                    }
                }
            }
            htmlTxt.append("</td></tr>");
            htmlTxt.append("</table>");
        }
        // print out
        win->b->value(htmlTxt.c_str());
        win->b->redraw();
    }
    usleep(200000);
    // clean up and exit
    if (win->thread==nullptr || error)
    {
        win->rqstNum = 0;
        win->msgProcessor->deleteOldRqst(requestNumber);
        win->thread = nullptr;
    }
    else win->msgProcessor->ignoreOldRqst(requestNumber);
    Fl::flush();
    pthread_exit(NULL);
}

unsigned long long ShowDecThreadW::printOutThread(list<pair<Type12Entry*, CIDsSet*>>* thread, KeysHandling *keys, LiquiditiesHandling *liquis, string &htmlTxt)
{
    if (thread==nullptr || thread->size()<=0)
    {
        htmlTxt="Error: Missing thread information.";
        return ULLONG_MAX;
    }
    Util u;
    htmlTxt.append("<table border=1 bordercolor=\"#000000\" cellspacing=\"0\" cellpadding=\"2\" width=\"100%\">");
    CIDsSet* iDsOfPrevious = nullptr;
    unsigned long long status=0;
    bool error = false;
    unsigned long long processingTime=0;
    unsigned long long lastChangeTime=0;
    string liquiName;
    if (liquis == nullptr && thread->begin()->first->underlyingType() != 3)
    {
        htmlTxt="Error: Missing liquidities information.";
        return ULLONG_MAX;
    }
    list<pair<Type12Entry*, CIDsSet*>>::iterator it;
    for (it=thread->begin(); it!=thread->end(); ++it)
    {
        // check
        if (status==ULLONG_MAX)
        {
            htmlTxt="Error: Unexpected entry.";
            error=true;
            break;
        }
        // get entry
        Type12Entry* t12e = it->first;
        CompleteID firstId = it->second->first();
        // determine type
        if (t12e->underlyingType()==6)
        {
            // check
            if (it != thread->begin())
            {
                htmlTxt="Error: Unexpected type 6 entry.";
                error=true;
                break;
            }
            // build head
            htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\">");
            Type6Entry* t6e = (Type6Entry*) t12e->underlyingEntry();
            // report type
            htmlTxt.append("<b>Application type: referee application</b><br>\n");
            // report time
            htmlTxt.append("Application submission time: ");
            lastChangeTime=firstId.getTimeStamp();
            htmlTxt.append(u.epochToStr(lastChangeTime));
            htmlTxt.append("<br>\n");
            // report applicant
            htmlTxt.append("Applicant Id: <font face=\"courier\" size=\"2\">");
            CompleteID ownerID = t6e->getOwnerID();
            htmlTxt.append(ownerID.to27Char());
            string ownerName = keys->getNameByID(ownerID);
            if (ownerName.length()==0) htmlTxt.append("</font><br>\n");
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(ownerName);
                htmlTxt.append(")<br>\n");
            }
            // report liquidity
            htmlTxt.append("Currency Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(t6e->getCurrency().to27Char());
            liquiName = liquis->getNameById(t6e->getCurrency());
            if (liquiName.length()==0)
            {
                htmlTxt="Error: Unexpected currency ";
                htmlTxt.append(t6e->getCurrency().to27Char());
                htmlTxt.append(". Please assign name to this currency.");
                error=true;
                break;
            }
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(liquiName);
                htmlTxt.append(")<br>\n");
            }
            Type5Entry* t5e = (Type5Entry*) liquis->getLiqui(liquiName);
            if (t5e==nullptr)
            {
                htmlTxt="Error: Currency data unavailable.";
                error=true;
                break;
            }
            // begin of tenure
            htmlTxt.append("Scheduled begin of tenure: ");
            htmlTxt.append(u.epochToStr(t6e->getTenureStart()));
            htmlTxt.append("<br>\n");
            // report processing time
            processingTime = t5e->getRefApplProcessingTime();
            processingTime *= 1000;
            double processingTimeInDays = 1.0 / 60 / 60 / 24 * t5e->getRefApplProcessingTime();
            htmlTxt.append("Processing time (days): ");
            htmlTxt.append(u.toString(processingTimeInDays, 2));
            htmlTxt.append("<br>\n");
            // report referee stake
            htmlTxt.append("Referee stake: ");
            htmlTxt.append(u.toString(t5e->getRefApplStake(), 2));
            htmlTxt.append("<br>\n");
            // print description
            htmlTxt.append("<i>Description:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
            string* descriptionP = t6e->getDescription();
            if (descriptionP==nullptr)
            {
                htmlTxt.append("Error.");
            }
            else
            {
                string prefix("<!doctype creole>");
                if (descriptionP->compare(0, prefix.length(), prefix) == 0)
                {
                    size_t descriptionLength = descriptionP->length() - prefix.length();
                    string description = descriptionP->substr(prefix.length(), descriptionLength);
                    NME nme(description.c_str(), descriptionLength);
                    nme.setFormat(NMEOutputFormatHTML);
                    NMEConstText output;
                    if (nme.getOutput(&output) == kNMEErrOk)
                    {
                        htmlTxt.append((char*) output);
                    }
                    else htmlTxt.append("Error while parsing Creole.");
                }
                else htmlTxt.append(*descriptionP);
            }
            htmlTxt.append("</td></tr>");
            iDsOfPrevious=it->second;
        }
        else if (t12e->underlyingType()==7)
        {
            // check
            if (it != thread->begin())
            {
                htmlTxt="Error: Unexpected type 7 entry.";
                error=true;
                break;
            }
            // build head
            htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\">");
            Type7Entry* t7e = (Type7Entry*) t12e->underlyingEntry();
            // report type
            htmlTxt.append("<b>Application type: operation proposal</b><br>\n");
            // report time
            htmlTxt.append("Application submission time: ");
            lastChangeTime=firstId.getTimeStamp();
            htmlTxt.append(u.epochToStr(lastChangeTime));
            htmlTxt.append("<br>\n");
            // report applicant
            htmlTxt.append("Applicant Id: <font face=\"courier\" size=\"2\">");
            CompleteID ownerID = t7e->getOwnerID();
            htmlTxt.append(ownerID.to27Char());
            string ownerName = keys->getNameByID(ownerID);
            if (ownerName.length()==0) htmlTxt.append("</font><br>\n");
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(ownerName);
                htmlTxt.append(")<br>\n");
            }
            // report liquidity
            htmlTxt.append("Currency Id: <font face=\"courier\" size=\"2\">");
            htmlTxt.append(t7e->getCurrency().to27Char());
            liquiName = liquis->getNameById(t7e->getCurrency());
            if (liquiName.length()==0)
            {
                htmlTxt="Error: Unexpected currency ";
                htmlTxt.append(t7e->getCurrency().to27Char());
                htmlTxt.append(". Please assign name to this currency.");
                error=true;
                break;
            }
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(liquiName);
                htmlTxt.append(")<br>\n");
            }
            Type5Entry* t5e = (Type5Entry*) liquis->getLiqui(liquiName);
            if (t5e==nullptr)
            {
                htmlTxt="Error: Currency data unavailable.";
                error=true;
                break;
            }
            // report amount
            htmlTxt.append("Amount: ");
            htmlTxt.append(u.toString(t7e->getAmount(), 2));
            htmlTxt.append("<br>\n");
            // report fee
            htmlTxt.append("Fee: ");
            htmlTxt.append(u.toString(t7e->getFee(), 2));
            htmlTxt.append("<br>\n");
            // report forfeit
            htmlTxt.append("Forfeit: ");
            htmlTxt.append(u.toString(t7e->getOwnStake(), 2));
            htmlTxt.append("<br>\n");
            // report processing time
            processingTime = t7e->getProcessingTime();
            processingTime *= 1000;
            double processingTimeInDays = 1.0 / 60 / 60 / 24 * t7e->getProcessingTime();
            htmlTxt.append("Processing time (days): ");
            htmlTxt.append(u.toString(processingTimeInDays, 2));
            htmlTxt.append("<br>\n");
            // report referee stake
            htmlTxt.append("Referee stake: ");
            htmlTxt.append(u.toString(t5e->getRefereeStake(t7e->getAmount(), t7e->getFee()), 2));
            htmlTxt.append("<br>\n");
            // print description
            htmlTxt.append("<i>Description:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
            string* descriptionP = t7e->getDescription();
            if (descriptionP==nullptr)
            {
                htmlTxt.append("Error.");
            }
            else
            {
                string prefix("<!doctype creole>");
                if (descriptionP->compare(0, prefix.length(), prefix) == 0)
                {
                    size_t descriptionLength = descriptionP->length() - prefix.length();
                    string description = descriptionP->substr(prefix.length(), descriptionLength);
                    NME nme(description.c_str(), descriptionLength);
                    nme.setFormat(NMEOutputFormatHTML);
                    NMEConstText output;
                    if (nme.getOutput(&output) == kNMEErrOk)
                    {
                        htmlTxt.append((char*) output);
                    }
                    else htmlTxt.append("Error while parsing Creole.");
                }
                else htmlTxt.append(*descriptionP);
            }
            htmlTxt.append("</td></tr>");
            iDsOfPrevious=it->second;
        }
        else if (t12e->underlyingType()==8)
        {
            Type8Entry* t8e = (Type8Entry*) t12e->underlyingEntry();
            // check
            if (it == thread->begin())
            {
                htmlTxt="Error: Unexpected type 8 entry.";
                error=true;
                break;
            }
            else if (iDsOfPrevious==nullptr || !iDsOfPrevious->contains(t8e->getPreviousThreadEntry()))
            {
                htmlTxt="Error: Inconsistent type 8 entry.";
                error=true;
                break;
            }
            // build head
            if (status % 2 == 0)
            {
                htmlTxt.append("<tr><td bgcolor=\"#A6E88E\"><b>");
                htmlTxt.append(to_string(status/2+1));
                htmlTxt.append(". Approval</b><br>\n");
            }
            else
            {
                htmlTxt.append("<tr><td bgcolor=\"#E8A08E\"><b>");
                htmlTxt.append(to_string(status/2+1));
                htmlTxt.append(". Rejection</b><br>\n");
            }
            // report time
            htmlTxt.append("Time: ");
            lastChangeTime=firstId.getTimeStamp();
            htmlTxt.append(u.epochToStr(lastChangeTime));
            htmlTxt.append("<br>\n");
            // report referee
            htmlTxt.append("Referee Id: <font face=\"courier\" size=\"2\">");
            CompleteID refId = t12e->pubKeyID();
            htmlTxt.append(refId.to27Char());
            string refName = keys->getNameByID(refId);
            if (refName.length()==0) htmlTxt.append("</font><br>\n");
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(refName);
                htmlTxt.append(")<br>\n");
            }
            // report referee appointment
            string liquiCandidate = liquis->getNameById(t8e->getTerminationID());
            if (liquiName.compare(liquiCandidate) == 0)
                htmlTxt.append("Referee Appointment Id: <font face=\"courier\" size=\"2\">");
            else
                htmlTxt.append("Referee Appointment Id (decision thread): <font face=\"courier\" size=\"2\">");
            htmlTxt.append(t8e->getTerminationID().to27Char());
            htmlTxt.append("</font><br>\n");
            // print description
            htmlTxt.append("<i>Explanation:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
            string* descriptionP = t8e->getDescription();
            if (descriptionP==nullptr)
            {
                htmlTxt.append("Error.");
            }
            else
            {
                string prefix("<!doctype creole>");
                if (descriptionP->compare(0, prefix.length(), prefix) == 0)
                {
                    size_t descriptionLength = descriptionP->length() - prefix.length();
                    string description = descriptionP->substr(prefix.length(), descriptionLength);
                    NME nme(description.c_str(), descriptionLength);
                    nme.setFormat(NMEOutputFormatHTML);
                    NMEConstText output;
                    if (nme.getOutput(&output) == kNMEErrOk)
                    {
                        htmlTxt.append((char*) output);
                    }
                    else htmlTxt.append("Error while parsing Creole.");
                }
                else htmlTxt.append(*descriptionP);
            }
            htmlTxt.append("</td></tr>");
            iDsOfPrevious=it->second;
            status++;
        }
        else if (t12e->underlyingType()==9)
        {
            Type9Entry* t9e = (Type9Entry*) t12e->underlyingEntry();
            // check
            if (it == thread->begin())
            {
                htmlTxt="Error: Unexpected type 9 entry.";
                error=true;
                break;
            }
            else if (iDsOfPrevious==nullptr || !iDsOfPrevious->contains(t9e->getPreviousThreadEntry()))
            {
                htmlTxt="Error: Inconsistent type 9 entry.";
                error=true;
                break;
            }
            status=ULLONG_MAX;
        }
        else if (t12e->underlyingType()==3)
        {
            // check
            if (it != thread->begin())
            {
                htmlTxt="Error: Unexpected type 3 entry.";
                error=true;
                break;
            }
            // build head
            htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\">");
            Type3Entry* t3e = (Type3Entry*) t12e->underlyingEntry();
            // report type
            htmlTxt.append("<b>Application type: notary application</b><br>\n");
            // report time
            htmlTxt.append("Application submission time: ");
            lastChangeTime=firstId.getTimeStamp();
            htmlTxt.append(u.epochToStr(lastChangeTime));
            htmlTxt.append("<br>\n");
            // report applicant
            htmlTxt.append("Applicant Id: <font face=\"courier\" size=\"2\">");
            CompleteID ownerID = t3e->getOwnerID();
            htmlTxt.append(ownerID.to27Char());
            string ownerName = keys->getNameByID(ownerID);
            if (ownerName.length()==0) htmlTxt.append("</font><br>\n");
            else
            {
                htmlTxt.append("</font> (");
                htmlTxt.append(ownerName);
                htmlTxt.append(")<br>\n");
            }
            // report processing time
            unsigned short lineage = keys->getType1Entry()->getLineage(lastChangeTime);
            processingTime = keys->getType1Entry()->getProposalProcessingTime(lineage);
            processingTime *= 1000;
            double processingTimeInDays = 1.0 / 60 / 60 / 24 * keys->getType1Entry()->getProposalProcessingTime(lineage);
            htmlTxt.append("Processing time (days): ");
            htmlTxt.append(u.toString(processingTimeInDays, 2));
            htmlTxt.append("<br>\n");
            // print description
            htmlTxt.append("<i>Description:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
            string* descriptionP = t3e->getDescription();
            if (descriptionP==nullptr)
            {
                htmlTxt.append("Error.");
            }
            else
            {
                string prefix("<!doctype creole>");
                if (descriptionP->compare(0, prefix.length(), prefix) == 0)
                {
                    size_t descriptionLength = descriptionP->length() - prefix.length();
                    string description = descriptionP->substr(prefix.length(), descriptionLength);
                    NME nme(description.c_str(), descriptionLength);
                    nme.setFormat(NMEOutputFormatHTML);
                    NMEConstText output;
                    if (nme.getOutput(&output) == kNMEErrOk)
                    {
                        htmlTxt.append((char*) output);
                    }
                    else htmlTxt.append("Error while parsing Creole.");
                }
                else htmlTxt.append(*descriptionP);
            }
            htmlTxt.append("</td></tr>");
            iDsOfPrevious=it->second;
        }
        else if (t12e->underlyingType()==4)
        {
            Type4Entry* t4e = (Type4Entry*) t12e->underlyingEntry();
            // check
            if (it == thread->begin())
            {
                htmlTxt="Error: Unexpected type 4 entry.";
                error=true;
                break;
            }
            else if (iDsOfPrevious==nullptr || !iDsOfPrevious->contains(t4e->getPreviousThreadEntry()))
            {
                htmlTxt="Error: Inconsistent type 4 entry.";
                error=true;
                break;
            }
            // build head
            if (status % 2 == 0)
            {
                htmlTxt.append("<tr><td bgcolor=\"#A6E88E\"><b>");
                htmlTxt.append(to_string(status/2+1));
                htmlTxt.append(". Approval</b><br>\n");
            }
            else
            {
                htmlTxt.append("<tr><td bgcolor=\"#E8A08E\"><b>");
                htmlTxt.append(to_string(status/2+1));
                htmlTxt.append(". Rejection</b><br>\n");
            }
            // report time
            htmlTxt.append("Time: ");
            lastChangeTime=firstId.getTimeStamp();
            htmlTxt.append(u.epochToStr(lastChangeTime));
            htmlTxt.append("<br>\n");
            // report notary
            unsigned short lineage = keys->getType1Entry()->getLineage(lastChangeTime);
            htmlTxt.append("Refereeing notary (total notary number): ");
            htmlTxt.append(to_string(lineage));
            htmlTxt.append(",");
            htmlTxt.append(to_string(t4e->getNotaryNr()));
            htmlTxt.append("<br>\n");
            // print description
            htmlTxt.append("<i>Explanation:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
            string* descriptionP = t4e->getDescription();
            if (descriptionP==nullptr)
            {
                htmlTxt.append("Error.");
            }
            else
            {
                string prefix("<!doctype creole>");
                if (descriptionP->compare(0, prefix.length(), prefix) == 0)
                {
                    size_t descriptionLength = descriptionP->length() - prefix.length();
                    string description = descriptionP->substr(prefix.length(), descriptionLength);
                    NME nme(description.c_str(), descriptionLength);
                    nme.setFormat(NMEOutputFormatHTML);
                    NMEConstText output;
                    if (nme.getOutput(&output) == kNMEErrOk)
                    {
                        htmlTxt.append((char*) output);
                    }
                    else htmlTxt.append("Error while parsing Creole.");
                }
                else htmlTxt.append(*descriptionP);
            }
            htmlTxt.append("</td></tr>");
            iDsOfPrevious=it->second;
            status++;
        }
    }
    if (error) return ULLONG_MAX;
    if (status==ULLONG_MAX) return 0;
    return lastChangeTime+processingTime;
}

const char* ShowDecThreadW::linkRoutine(Fl_Widget *w, const char *uri)
{
    string str(uri);

    winByApplIdMutex.lock();
    if (winByApplId.count(str)<1)
    {
        winByApplIdMutex.unlock();
        return uri;
    }
    ShowDecThreadW* win=winByApplId[str];
    winByApplIdMutex.unlock();

    CompleteID applId(str);
    if (applId.getNotary()<=0 || win->thread==nullptr)
    {
        fl_alert("Error: %s", "please first reload this thread to proceed.");
        return NULL;
    }

    int lastEntryType = win->thread->rbegin()->first->underlyingType();
    if (lastEntryType == 6 || lastEntryType == 7 || lastEntryType == 8)
    {
        RefereeNoteW *refereeNoteW = new RefereeNoteW(win->subWins, win->internal, win->thread);
        win->subWins->insert(win->subWins->end(), (Fl_Window*)refereeNoteW);
        ((Fl_Window*)refereeNoteW)->show();
        win->thread=nullptr;
        return NULL;
    }
    else if (lastEntryType == 3 || lastEntryType == 4)
    {
        NotaryNoteW *notaryNoteW = new NotaryNoteW(win->subWins, win->internal, win->thread);
        win->subWins->insert(win->subWins->end(), (Fl_Window*)notaryNoteW);
        ((Fl_Window*)notaryNoteW)->show();
        win->thread=nullptr;
        return NULL;
    }
    else if (lastEntryType == 9 && win->thread->begin()->first->underlyingType() == 3)
    {
        CompleteID applicantId = win->thread->begin()->first->pubKeyID();
        string key = win->keys->getNameByID(applicantId);
        CompleteID terminationId = win->thread->rbegin()->second->last();
        // build request
        string rqst;
        unsigned long notaryNr = win->connection->getNotaryNr();
        unsigned long result = win->requestBuilder->createNotaryRegistrationRequest(key, terminationId, notaryNr, rqst);
        // final checks
        if (result<=0)
        {
            fl_alert("Error: %s", "insufficient data to process request.");
            return NULL;
        }
        Util u;
        Type1Entry *t1e = win->keys->getType1Entry();
        unsigned long long tenureStart = t1e->getNotaryTenureStartCorrected(t1e->latestLin(), result);
        int answer = fl_choice("Are you sure you want to accept notary status for the account %s\nand become notary with the number %s\nwith tenure starting on %s?",
                               "Close", "No", "Yes", key.c_str(), to_string(result).c_str(), u.epochToStr(tenureStart).c_str());
        if (answer != 2) return NULL;

        // rebuild request
        rqst="";
        unsigned long result2 = win->requestBuilder->createNotaryRegistrationRequest(key, terminationId, notaryNr, rqst);
        if (result2<=0)
        {
            fl_alert("Error: %s", "insufficient data to process request.");
            return NULL;
        }
        if (result!=result2)
        {
            fl_alert("Error: %s", "lineage data appears to have changed.");
            return NULL;
        }

        // send request
        win->connection->sendRequest(rqst);
        fl_alert("Info: %s", "a new lineage entry was sent for notarization.");
    }
    return NULL;
}
