#include "ShowLineageInfoW.h"

InternalThread* ShowLineageInfoW::internal;
KeysHandling* ShowLineageInfoW::keys;
list<Fl_Window*>* ShowLineageInfoW::subWins;

ShowLineageInfoW::ShowLineageInfoW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(580, 480, "Lineage information"), waitForDataThread(nullptr)
{
    internal=i;
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum = 0;

    // create html area
    b = new Fl_Html_View(20, 20, 540, 400, 5);
    b->color(FL_WHITE);
    b->format_width(-1);
    b->value("<p></p>");

    // application button
    but1 = new Fl_Button(20,435,150,25,"Notary application");
    but1->callback(onApplication, (void*)this);

    // reload button
    but2 = new Fl_Button(450,435,110,25,"Reload data");
    but2->callback(onReload, (void*)this);

    end();
    resizable(b);

    flush();

    onReload((Fl_Widget*) but2, (void*)this);
}

ShowLineageInfoW::~ShowLineageInfoW()
{
    if (waitForDataThread!=nullptr) delete waitForDataThread;
}

void ShowLineageInfoW::onApplication(Fl_Widget *w, void *d)
{
    //ShowLineageInfoW *win = (ShowLineageInfoW*)d;
    NotaryApplicationW *notaryApplicationW = new NotaryApplicationW(subWins, internal);
    subWins->insert(subWins->end(), (Fl_Window*)notaryApplicationW);
    ((Fl_Window*)notaryApplicationW)->show();
}

void ShowLineageInfoW::onReload(Fl_Widget *w, void *d)
{
    ShowLineageInfoW *win = (ShowLineageInfoW*)d;
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
    // build and send request
    keys->lock();
    CompleteID lastT2EIdFirst = keys->getLastT2eIdFirst();
    keys->unlock();
    string rqst;
    bool result = win->requestBuilder->createEssentialsRqst(lastT2EIdFirst, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForDataThread!=nullptr) delete win->waitForDataThread;
    win->waitForDataThread = new pthread_t();
    if (pthread_create(win->waitForDataThread, NULL, ShowLineageInfoW::waitForDataRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForDataThread));
}

void* ShowLineageInfoW::waitForDataRoutine(void *w)
{
    ShowLineageInfoW* win=(ShowLineageInfoW*) w;
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

    if (entriesList!=nullptr)
    {
        // display lineage data from KeysHandling
        Util u;
        string htmlTxt;
        Type1Entry *t1e = win->keys->getType1Entry();
        unsigned int maxLin = t1e->latestLin();
        for (unsigned int i=0; i<maxLin; i++)
        {
            // report sub-lineage number
            htmlTxt.append("<b>Sub-lineage  ");
            htmlTxt.append(to_string(i+1));
            htmlTxt.append("</b><br>&nbsp;<br>\n");
            // report max acting
            htmlTxt.append("max. number of acting notaries: ");
            htmlTxt.append(to_string(t1e->getMaxActing(i+1)));
            htmlTxt.append("<br>\n");
            // report signatures in each notarization
            htmlTxt.append("Signatures in each notarization: ");
            htmlTxt.append(to_string(t1e->getNeededSignatures(i+1)));
            htmlTxt.append("<br>\n");
            // report max. number of waiting
            htmlTxt.append("max. number of waiting notaries: ");
            htmlTxt.append(to_string(t1e->getMaxWaiting(i+1)));
            htmlTxt.append("<br>\n");
            // report tenure duration
            htmlTxt.append("Notary tenure duration in seconds: ");
            htmlTxt.append(to_string(t1e->getNotaryTenure(i+1)));
            htmlTxt.append("<br>\n");
            // report maxNotarizationTime
            htmlTxt.append("max. notarization time in milliseconds: ");
            htmlTxt.append(to_string(t1e->getMaxNotarizationTimeByLineage(i+1)));
            htmlTxt.append("<br>\n");
            // report freshnessTime
            htmlTxt.append("Freshness time in seconds: ");
            htmlTxt.append(to_string(t1e->getFreshnessTime(i+1)));
            htmlTxt.append("<br>\n");
            // report proposalProcessingTime
            htmlTxt.append("Notary application processing time in seconds: ");
            htmlTxt.append(to_string(t1e->getProposalProcessingTime(i+1)));
            htmlTxt.append("<br>\n");
            // report notaryTenureStart
            htmlTxt.append("First notary formal tenure start (unix epoch in ms): ");
            htmlTxt.append(to_string(t1e->getNotaryTenureStartFormal(i+1,1)));
            htmlTxt.append("<br>\n");
            // notarization fee
            htmlTxt.append("Notarization fee (in %): ");
            htmlTxt.append(u.toString(t1e->getFee(i+1), 2));
            htmlTxt.append("<br>\n");
            // first penalty parameter
            htmlTxt.append("First penalty factor: ");
            htmlTxt.append(u.toString(t1e->getPenaltyFactor1(i+1), 2));
            htmlTxt.append("<br>\n");
            // second penalty parameter
            htmlTxt.append("Second penalty factor: ");
            htmlTxt.append(u.toString(t1e->getPenaltyFactor2(i+1), 2));
            htmlTxt.append("<br>\n");
            // report approved notaries
            htmlTxt.append("Number of notaries appointed via application so far: ");
            htmlTxt.append(to_string(win->keys->getAppointedNotariesNum(i+1)));
            htmlTxt.append("<br>\n");
            // cutoff related info
            if (i+1<maxLin)
            {
                // report cut-off time
                htmlTxt.append("Sub-lineage cut-off time (unix epoch in ms): ");
                htmlTxt.append(to_string(t1e->getCutOff(i+1)));
                htmlTxt.append("<br>\n");
                // report extension time
                htmlTxt.append("Freshness extension (in ms): ");
                htmlTxt.append(to_string(t1e->getExtension(i+1)));
                htmlTxt.append("<br>\n");
                // report kickoff time
                htmlTxt.append("Next sub-lineage kick-off time (unix epoch in ms): ");
                htmlTxt.append(to_string(t1e->getKickOff(i+1)));
                htmlTxt.append("<br>\n<hr>\n");
            }
        }
        // print out
        win->b->value(htmlTxt.c_str());
        win->b->redraw();
    }
    usleep(200000);
    win->msgProcessor->deleteOldRqst(requestNumber);
    win->rqstNum = 0;
    Fl::flush();
    pthread_exit(NULL);
}
