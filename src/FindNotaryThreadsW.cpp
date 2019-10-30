#include "FindNotaryThreadsW.h"

#define timeLag 2419200000 // four weeks

list<Fl_Window*>* FindNotaryThreadsW::subWins;

FindNotaryThreadsW::FindNotaryThreadsW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(600, 480, "Find decision threads"),
    internal(i), waitForListThread(nullptr)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    msgProcessor = connection->getMsgProcessor();
    rqstNum=0;

    // choose applicant
    box2 = new Fl_Box(20,10,196,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box2->label("Applicant (optional):");
    ch2 = new Fl_Choice(220,10,220,25);
    ch2->add(" ");
    list<string> publicKeysNames;
    keys->loadPublicKeysNames(publicKeysNames);
    list<string>::iterator it;
    for (it=publicKeysNames.begin(); it!=publicKeysNames.end(); ++it)
    {
        ch2->add(it->c_str());
    }
    ch2->take_focus();
    ch2->value(0);

    // choose referee
    box3 = new Fl_Box(20,40,196,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box3->label("Participant (optional):");
    ch3 = new Fl_Choice(220,40,220,25);
    ch3->add(" ");
    for (it=publicKeysNames.begin(); it!=publicKeysNames.end(); ++it)
    {
        ch3->add(it->c_str());
    }
    ch3->take_focus();
    ch3->value(0);

    // choose statis
    box4 = new Fl_Box(20,70,196,25);
    box4->labelfont(FL_COURIER);
    box4->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box4->label("Status:");
    ch4 = new Fl_Choice(220,70,220,25);
    ch4->add("New application");
    ch4->add("Pending approval");
    ch4->add("Pending rejection");
    ch4->add("Approved");
    ch4->add("Rejected");
    ch4->take_focus();
    ch4->value(0);

    // set min application id
    box5 = new Fl_Box(20,100,196,25);
    box5->labelfont(FL_COURIER);
    box5->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    box5->label("min. application time:");
    inp = new Fl_Input(220,100,220,25);
    inp->textsize(11);
    inp->textfont(FL_COURIER);
    Util u;
    unsigned long long currentTime = requestBuilder->currentSyncTimeInMs();
    if (currentTime>=timeLag) currentTime-=timeLag;
    inp->value(u.epochToStr2(currentTime).c_str());

    // request button
    but1 = new Fl_Button(470,100,110,25,"Request data");
    but1->callback(onRequest, (void*)this);

    // create html area
    view = new Fl_Html_View(20, 130, 560, 330, 0);
    view->color(FL_WHITE);
    view->format_width(-1);
    view->value("<p></p>");

    end();
    resizable(view);
}

FindNotaryThreadsW::~FindNotaryThreadsW()
{
    if (waitForListThread!=nullptr) delete waitForListThread;
}

void FindNotaryThreadsW::onRequest(Fl_Widget *w, void *d)
{
    FindNotaryThreadsW *win = (FindNotaryThreadsW*)d;
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
    if (win->ch2->text()==nullptr || win->ch3->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // get applicant
    string applicant(win->ch2->text());
    if (applicant.compare(" ")==0) applicant="";
    // get referee
    string referee(win->ch3->text());
    if (referee.compare(" ")==0) referee="";
    if (applicant.length()>0 && referee.length()>0)
    {
        fl_alert("Error: %s", "applicant and refereeing notary cannot be specified at the same time.");
        return;
    }
    // get status
    unsigned char status = win->ch4->value();

    // get minimal time and build respective minApplId
    string str = string(win->inp->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no minimal time supplied.");
        return;
    }
    unsigned long long minTime = 0;
    try
    {
        Util u;
        minTime = u.str2ToEpoch(str);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad minimal time supplied.");
        return;
    }
    if (minTime<=0)
    {
        fl_alert("Error: %s", "bad minimal time supplied.");
        return;
    }
    CompleteID minApplId(1, 0, minTime);

    // determine type
    unsigned char spec = status;
    if (referee.length()>0)
    {
        spec+=10;
        applicant=referee;
    }
    // build and send request
    string rqst;
    bool result = win->requestBuilder->createDecThrInfoRqst(spec, applicant, minApplId, 14, rqst);
    if (result)
    {
        win->rqstNum = win->msgProcessor->addRqstAnticipation(rqst);
        win->connection->sendRequest(rqst);
    }
    // wait for answer
    if (win->rqstNum <= 0) return;
    if (win->waitForListThread!=nullptr) delete win->waitForListThread;
    win->waitForListThread = new pthread_t();
    if (pthread_create((pthread_t*)win->waitForListThread, NULL, FindNotaryThreadsW::waitForListRoutine, (void*) win) < 0)
    {
        return;
    }
    pthread_detach(*(win->waitForListThread));
}

void* FindNotaryThreadsW::waitForListRoutine(void *w)
{
    FindNotaryThreadsW* win=(FindNotaryThreadsW*) w;
    unsigned long long requestNumber = win->rqstNum;
    win->view->value("<h3>Loading ...</h3>");
    win->view->redraw();
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
    if (entriesList==nullptr)
    {
        win->view->value("<h3>Request timeout. No data received.</h3>");
        win->view->redraw();
    }
    else if (entriesList->size()==0)
    {
        win->view->value("<h3>No applications reported.</h3>");
        win->view->redraw();
    }
    else if (entriesList->begin()->first->underlyingType()==3)
    {
        string htmlTxt;
        // build table head
        htmlTxt.append("<table border=2 bordercolor=\"#FFFFFF\"><tr>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Creation Time</td>");
        htmlTxt.append("<td bgcolor=\"#E8E8E8\" nowrap>Application ID</td>");
        htmlTxt.append("</tr>\n");
        // create rows with applications info
        Util u;
        list<pair<Type12Entry*, CIDsSet*>>::iterator it;
        for (it=entriesList->begin(); it!=entriesList->end(); ++it)
        {
            //Type3Entry* t3e = (Type3Entry*) it->first->underlyingEntry();
            CompleteID id = it->second->first();
            htmlTxt.append("<tr>");
            string tdStart("<td bgcolor=\"#F0F0F0\" ");
            // date and time
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font size=\"2\">");
            htmlTxt.append(u.epochToStr(id.getTimeStamp()));
            htmlTxt.append("</font></td>");
            // Application ID
            htmlTxt.append(tdStart);
            htmlTxt.append("align=\"left\" valign=\"top\" nowrap><font face=\"courier\" size=\"2\">");
            htmlTxt.append(id.to27Char());
            htmlTxt.append("</font></td>");
        }
        htmlTxt.append("</table>");

        win->view->value(htmlTxt.c_str());
        win->view->redraw();
    }
    else
    {
        win->view->value("<h3>Error: unknown data type.</h3>");
        win->view->redraw();
    }
    usleep(200000);
    // clean up and exit
    Fl::flush();
    win->msgProcessor->deleteOldRqst(requestNumber);
    win->rqstNum = 0;
    return NULL;
}
