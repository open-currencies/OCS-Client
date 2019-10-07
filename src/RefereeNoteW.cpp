#include "RefereeNoteW.h"

list<Fl_Window*>* RefereeNoteW::subWins;

RefereeNoteW::RefereeNoteW(list<Fl_Window*> *s, InternalThread *i, list<pair<Type12Entry*, CIDsSet*>>* t) : Fl_Window(640, 450, "Note"),
    internal(i), thread(t), stake(-1)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();
    msgProcessor = connection->getMsgProcessor();

    // determine type
    refereeApplication=false;
    if (thread->begin()->first->underlyingType()==6) refereeApplication=true;

    // get application id
    applicationId=thread->begin()->second->first();

    // get thread id
    threadId=thread->back().second->last();

    // set title
    string title("Approving note for ");
    if (thread->size() % 2 == 0) title = "Rejecting note for ";
    if (refereeApplication) title.append("referee application ");
    else title.append("operation proposal ");
    title.append(applicationId.to27Char());
    copy_label(title.c_str());

    // get liquidity name
    if (refereeApplication)
    {
        Type6Entry* t6e = (Type6Entry*) thread->begin()->first->underlyingEntry();
        CompleteID currID = t6e->getCurrency();
        liqui=liquis->getNameById(currID);
        amount=0;
        fee=0;
    }
    else
    {
        Type7Entry* t7e = (Type7Entry*) thread->begin()->first->underlyingEntry();
        CompleteID currID = t7e->getCurrency();
        liqui=liquis->getNameById(currID);
        amount=t7e->getAmount();
        fee=t7e->getFee();
    }

    // choose referee
    group0 = new Fl_Group(20, 10, 400, 25);
    box0 = new Fl_Box(20,10,100,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box0->label("Referee:");
    ch0 = new Fl_Choice(120,10,240,25);
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
    dum0 = new Fl_Box(415,10,5,25);
    dum0->hide();
    group0->resizable(dum0);
    group0->end();

    // edit area
    group5 = new Fl_Group(20, 50, 180, 25);
    box5 = new Fl_Box(20,50,100,25);
    box5->labelfont(FL_COURIER_ITALIC);
    box5->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box5->label("Please use ");
    but1 = new Fl_Button(108,50,65,25,"Creole:");
    but1->labelfont(FL_COURIER_ITALIC);
    but1->labelcolor(fl_rgb_color(0,0,255));
    but1->box(FL_NO_BOX);
    but1->callback(onCreole, (void*)this);
    dum5 = new Fl_Box(175,50,5,25);
    dum5->hide();
    group5->resizable(dum5);
    group5->end();

    textbuf = new Fl_Text_Buffer;
    editor = new Fl_Text_Editor(20, 80, 600, 325);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);
    editor->wrap_mode(1, 3000);
    but2 = new Fl_Button(500,50,120,25,"Load from file");
    but2->callback(onLoad, (void*)this);
    but3 = new Fl_Button(20,415,110,25,"Save to file");
    but3->callback(onSave, (void*)this);
    but4 = new Fl_Button(540,415,80,25,"Proceed");
    but4->callback(onProceed, (void*)this);
    but4->deactivate();

    // create html area
    box6 = new Fl_Box(20,50,100,25);
    box6->labelfont(FL_COURIER);
    box6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box6->label("Note text:");
    box6->hide();
    view = new Fl_Html_View(20, 80, 600, 325, 0);
    view->color(FL_WHITE);
    view->format_width(-1);
    view->value("<p></p>");
    view->hide();
    but5 = new Fl_Button(540,415,80,25,"Back");
    but5->callback(onBack, (void*)this);
    but5->hide();
    but6 = new Fl_Button(290,415,60,25,"SEND");
    but6->callback(onSend, (void*)this);
    but6->hide();

    end();
    resizable(editor);

    dataStatusClear=false;

    // update liqui
    CompleteID liquiID = liquis->getID(liqui);
    internal->addIdOfInterest(liquiID);
    // check account state
    if (ch0->text()!=nullptr) referee=string(ch0->text());
    CompleteID keyID = keys->getInitialID(referee);
    internal->addIdOfInterest(keyID);
    internal->addAccountPairOfInterest(referee, liqui);

    // start check thread
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

RefereeNoteW::~RefereeNoteW()
{
    msgProcessor->deleteList(thread);
}

void RefereeNoteW::onChoice(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    string str(win->ch0->text());
    if (str.compare(win->referee) != 0)
    {
        win->dataStatusClear=false;
        win->but4->deactivate();

        win->referee=str;
        CompleteID keyID = win->keys->getInitialID(win->referee);
        win->internal->addIdOfInterest(keyID);
        win->internal->addAccountPairOfInterest(win->referee, win->liqui);
    }
}

void RefereeNoteW::onCreole(Fl_Widget *w, void *d)
{
    CreoleCheatSheatW *creoleCheatSheatW = new CreoleCheatSheatW();
    subWins->insert(subWins->end(), (Fl_Window*)creoleCheatSheatW);
    ((Fl_Window*)creoleCheatSheatW)->show();
}

void RefereeNoteW::onLoad(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    char *newfile;
    newfile = fl_file_chooser("Load text from...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->loadfile(newfile);
}

void RefereeNoteW::onSave(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    char *newfile;
    newfile = fl_file_chooser("Save text as...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->savefile(newfile);
}

void RefereeNoteW::onBack(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    win->view->hide();
    win->but5->hide();
    win->but6->hide();
    win->box6->hide();

    win->ch0->activate();

    win->editor->show();
    win->box5->show();
    win->but1->show();
    win->but2->show();
    win->but3->show();
    win->but4->show();
}

void RefereeNoteW::onProceed(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    // load type 5 entry
    Type5Entry *type5entry = (Type5Entry*) win->liquis->getLiqui(win->liqui);
    if (type5entry==nullptr)
    {
        fl_alert("Error: %s", "unknown currency.");
        return;
    }
    if (win->refereeApplication) win->stake = type5entry->getRefApplStake();
    else win->stake = type5entry->getRefereeStake(win->amount, win->fee);

    // get referee info
    RefereeInfo* refInfo = win->requestBuilder->getRefInfo(win->referee, win->liqui);
    if (refInfo==nullptr)
    {
        fl_alert("Error: insufficient data to process request.");
        return;
    }
    // check if ref is acting
    unsigned long long tenureEnd = refInfo->getTenureEnd();
    unsigned long long tenureStart = type5entry->getRefereeTenure();
    tenureStart*=1000;
    if (tenureStart<tenureEnd) tenureStart = tenureEnd - tenureStart;
    unsigned long long currentTime = win->requestBuilder->currentSyncTimeInMs();
    if (tenureStart>currentTime || currentTime>=tenureEnd)
    {
        fl_alert("Error: referee not confirmed as acting.");
        return;
    }
    // check if ref already took part
    list<pair<Type12Entry*, CIDsSet*>>::iterator it;
    for (it=win->thread->begin(); it!=win->thread->end(); ++it)
    {
        CompleteID pubKeyId = it->first->pubKeyID();
        if (win->referee.compare(win->keys->getNameByID(pubKeyId))==0)
        {
            fl_alert("Error: referee appears to have taken part in thread.");
            return;
        }
    }
    // check ref limits
    if (!win->refereeApplication && refInfo->getLiquidityCreated() >= type5entry->getLiquidityCreationLimit())
    {
        fl_alert("Error: liquidity approval limit out of bounds.");
        return;
    }
    if (win->refereeApplication && refInfo->getRefsAppointed() >= type5entry->getRefAppointmentLimit())
    {
        fl_alert("Error: referee appointment limit out of bounds.");
        return;
    }

    // display existing thread data
    string htmlTxt;
    unsigned long long terminationTime = ShowDecThreadW::printOutThread(win->thread, win->keys, win->liquis, htmlTxt);
    if (terminationTime==ULLONG_MAX)
    {
        fl_alert("Error in printing out updated thread.");
        return;
    }
    // build head for new note
    if (win->thread->size() % 2 == 1)
    {
        htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\"><b>");
        htmlTxt.append(to_string(win->thread->size()/2+1));
        htmlTxt.append(". Approval</b><br>\n");
    }
    else
    {
        htmlTxt.append("<tr><td bgcolor=\"#DEDEDE\"><b>");
        htmlTxt.append(to_string(win->thread->size()/2));
        htmlTxt.append(". Rejection</b><br>\n");
    }
    // report referee appointment
    string liquiCandidate = win->liquis->getNameById(refInfo->getAppointmentId());
    if (win->liqui.compare(liquiCandidate) == 0)
        htmlTxt.append("Referee Appointment Id: <font face=\"courier\" size=\"2\">");
    else
        htmlTxt.append("Referee Appointment Id (decision thread): <font face=\"courier\" size=\"2\">");
    htmlTxt.append(refInfo->getAppointmentId().to27Char());
    htmlTxt.append("</font><br>\n");
    // print description
    htmlTxt.append("<i>Explanation:</i></td></tr>\n<tr><td bgcolor=\"#FFFFFF\">");
    // add note to htmlTxt
    NME nme(win->textbuf->text());
    nme.setFormat(NMEOutputFormatHTML);
    NMEConstText output;
    if (nme.getOutput(&output) == kNMEErrOk)
    {
        htmlTxt.append((char*) output);
    }
    else
    {
        fl_alert("Error while parsing Creole.");
        return;
    }
    htmlTxt.append("</td></tr></table>");

    win->view->value(htmlTxt.c_str());
    win->ch0->deactivate();

    win->box5->hide();
    win->but1->hide();
    win->but2->hide();
    win->but3->hide();
    win->but4->hide();
    win->editor->hide();

    win->view->redraw();
    win->view->show();
    win->but5->show();
    win->but6->show();
    win->box6->show();
}

void RefereeNoteW::onSend(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    if (win->stake<0) return;
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    // build description
    string description("<!doctype creole>");
    description.append(win->textbuf->text());
    Util u;
    if (u.descriptionTooLarge(description))
    {
        fl_alert("Error: %s %u %s", "description too large. Limit:", u.descriptionLimitInKB(), "KB");
        return;
    }
    // build request
    string rqst;
    unsigned long notaryNr = win->connection->getNotaryNr();
    bool result = win->requestBuilder->createRefNoteRequest(win->referee, win->liqui, win->threadId,
                  description, notaryNr, rqst);

    // final checks
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }
    int answer = fl_choice("Are you sure you want to submit a new note for the referee\n%s and that all displayed data is correct?\nNote that a participation stake of %s units\nof currency %s will be withheld.",
                           "Close", "No", "Yes", win->referee.c_str(), u.toString(win->stake,2).c_str(), win->liqui.c_str());
    if (answer != 2) return;

    // rebuild request
    rqst="";
    result = win->requestBuilder->createRefNoteRequest(win->referee, win->liqui, win->threadId,
             description, notaryNr, rqst);
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }

    // send request
    win->connection->sendRequest(rqst);
    fl_alert("Info: %s", "a new referee note was sent for notarization.");
    win->hide();
}

void* RefereeNoteW::checkDataRoutine(void *w)
{
    RefereeNoteW* win=(RefereeNoteW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        Type5Or15Entry *t5Or15e =  win->liquis->getLiqui(win->liqui);
        if (t5Or15e==nullptr)
        {
            win->dataStatusClear = false;
            usleep(500000);
            continue;
        }
        else if (t5Or15e->getType()==15)
        {
            win->dataStatusClear=true;
            win->checkThreadRunning=false;
            pthread_exit(NULL);
        }

        if (win->refereeApplication) win->stake = ((Type5Entry*) t5Or15e)->getRefApplStake();
        else win->stake = ((Type5Entry*) t5Or15e)->getRefereeStake(win->amount, win->fee);

        RefereeInfo* refInfo = win->requestBuilder->getRefInfo(win->referee, win->liqui);
        CompleteID appointmentId;
        if (refInfo!=nullptr)
        {
            unsigned long long tenureEnd = refInfo->getTenureEnd();
            unsigned long long tenureStart = ((Type5Entry*) t5Or15e)->getRefereeTenure();
            tenureStart*=1000;
            if (tenureStart<tenureEnd) tenureStart = tenureEnd - tenureStart;
            unsigned long long currentTime = win->requestBuilder->currentSyncTimeInMs();
            if (tenureStart<=currentTime && currentTime<tenureEnd) appointmentId = refInfo->getAppointmentId();
        }

        if (appointmentId.getNotary()>0)
        {
            Type14Entry *t14e = win->requestBuilder->getLatestT14Ecopy(win->referee, win->liqui);
            if (t14e!=nullptr)
            {
                win->dataStatusClear = true;
                if (t14e->getTotalAmount() >= win->stake) win->but4->activate();
                delete t14e;
            }
            else win->dataStatusClear = false;
        }
        else win->dataStatusClear = false;

        usleep(500000);
    }
    pthread_exit(NULL);
}

void RefereeNoteW::onClose(Fl_Widget *w, void *d)
{
    RefereeNoteW *win = (RefereeNoteW*)d;
    win->checkThreadRunning=false;
    win->hide();
}
