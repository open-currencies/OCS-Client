#include "NotaryNoteW.h"

list<Fl_Window*>* NotaryNoteW::subWins;

NotaryNoteW::NotaryNoteW(list<Fl_Window*> *s, InternalThread *i, list<pair<Type12Entry*, CIDsSet*>>* t) : Fl_Window(640, 450, "Note"),
    internal(i), thread(t)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    msgProcessor = connection->getMsgProcessor();

    // get application id
    applicationId=thread->begin()->second->first();

    // get thread id
    threadId=thread->back().second->last();

    // set title
    string title("Approving note for ");
    if (thread->size() % 2 == 0) title = "Rejecting note for ";
    title.append("notary application ");
    title.append(applicationId.to27Char());
    copy_label(title.c_str());

    // choose referee
    group0 = new Fl_Group(20, 10, 400, 25);
    box0 = new Fl_Box(20,10,160,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box0->label("Refereeing notary:");
    ch0 = new Fl_Choice(180,10,240,25);
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

    // check account state
    if (ch0->text()!=nullptr) referee=string(ch0->text());
    // request notary info
    string rqst;
    string *publicKeyStr = keys->getPublicKeyStr(referee);
    if (publicKeyStr!=nullptr && requestBuilder->createNotaryInfoRqst(publicKeyStr, rqst))
        connection->sendRequest(rqst);

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

NotaryNoteW::~NotaryNoteW()
{
    msgProcessor->deleteList(thread);
}

void NotaryNoteW::onChoice(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
    string str(win->ch0->text());
    if (str.compare(win->referee) != 0)
    {
        win->dataStatusClear=false;
        win->but4->deactivate();
        win->referee=str;
        // request notary info
        string rqst;
        string *publicKeyStr = win->keys->getPublicKeyStr(str);
        if (publicKeyStr!=nullptr && win->requestBuilder->createNotaryInfoRqst(publicKeyStr, rqst))
            win->connection->sendRequest(rqst);
    }
}

void NotaryNoteW::onCreole(Fl_Widget *w, void *d)
{
    CreoleCheatSheatW *creoleCheatSheatW = new CreoleCheatSheatW();
    subWins->insert(subWins->end(), (Fl_Window*)creoleCheatSheatW);
    ((Fl_Window*)creoleCheatSheatW)->show();
}

void NotaryNoteW::onLoad(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
    char *newfile;
    newfile = fl_file_chooser("Load text from...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->loadfile(newfile);
}

void NotaryNoteW::onSave(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
    char *newfile;
    newfile = fl_file_chooser("Save text as...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->savefile(newfile);
}

void NotaryNoteW::onBack(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
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

void NotaryNoteW::onProceed(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;

    // check that notary is acting
    string *pubKey = win->keys->getPublicKeyStr(win->referee);
    TNtrNr tNtrNr = win->keys->getNotary(pubKey);
    if (tNtrNr.isZero())
    {
        fl_alert("Error: notary not found.");
        return;
    }
    Type1Entry *t1e = win->keys->getType1Entry();
    unsigned long long currentTimeInMs = win->requestBuilder->currentSyncTimeInMs();
    if (currentTimeInMs == 0 || tNtrNr.getLineage() != t1e->latestLin()
            || !t1e->isActingNotary(tNtrNr.getNotaryNr(), currentTimeInMs))
    {
        fl_alert("Error: notary not confirmed as acting.");
        return;
    }
    if (t1e->getLineage(win->applicationId.getTimeStamp()) != t1e->latestLin())
    {
        fl_alert("Error: application from previous lineage.");
        return;
    }

    if (win->thread->size()==1)
    {
        // get notary info
        NotaryInfo* notaryInfo = win->requestBuilder->getNotaryInfo(win->referee);
        if (notaryInfo==nullptr)
        {
            fl_alert("Error: insufficient data to process request.");
            return;
        }
        unsigned short lineage = t1e->getLineage(currentTimeInMs);
        if (notaryInfo->getInitiatedThreadsNum() > t1e->getMaxActing(lineage))
        {
            fl_alert("Error: threads initiation limit out of bounds.");
            return;
        }
    }

    // check if notary already took part
    list<pair<Type12Entry*, CIDsSet*>>::iterator it;
    for (it=win->thread->begin(); it!=win->thread->end(); ++it)
    {
        CompleteID pubKeyId = it->first->pubKeyID();
        if (win->referee.compare(win->keys->getNameByID(pubKeyId))==0)
        {
            fl_alert("Error: notary appears to have taken part in thread.");
            return;
        }
    }

    // display existing thread data
    string htmlTxt;
    unsigned long long terminationTime = ShowDecThreadW::printOutThread(win->thread, win->keys, nullptr, htmlTxt);
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
    // report participating notary
    htmlTxt.append("Refereeing notary (total notary number): ");
    htmlTxt.append(to_string(tNtrNr.getLineage()));
    htmlTxt.append(",");
    htmlTxt.append(to_string(tNtrNr.getNotaryNr()));
    htmlTxt.append("<br>\n");
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

void NotaryNoteW::onSend(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
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
    bool result = win->requestBuilder->createNotaryNoteRequest(win->referee, win->threadId, description, notaryNr, rqst);
    // final checks
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }
    int answer = fl_choice("Are you sure you want to submit a new refereeing note for the notary\n%s and that all displayed data is correct?",
                           "Close", "No", "Yes", win->referee.c_str());
    if (answer != 2) return;
    // rebuild request
    rqst="";
    result = win->requestBuilder->createNotaryNoteRequest(win->referee, win->threadId, description, notaryNr, rqst);
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }

    // send request
    win->connection->sendRequest(rqst);
    fl_alert("Info: %s", "a new notary note was sent for notarization.");
    win->hide();
}

void* NotaryNoteW::checkDataRoutine(void *w)
{
    NotaryNoteW* win=(NotaryNoteW*) w;
    win->dataStatusClear = false;
    while (win->checkThreadRunning)
    {
        if (win->dataStatusClear)
        {
            usleep(500000);
            continue;
        }

        string *pubKey = win->keys->getPublicKeyStr(win->referee);
        TNtrNr tNtrNr = win->keys->getNotary(pubKey);
        NotaryInfo* notaryInfo = win->requestBuilder->getNotaryInfo(win->referee);

        if (!tNtrNr.isZero() && notaryInfo!=nullptr)
        {
            win->dataStatusClear = true;
            win->but4->activate();
        }
        else win->dataStatusClear = false;

        usleep(500000);
    }
    return NULL;
}

void NotaryNoteW::onClose(Fl_Widget *w, void *d)
{
    NotaryNoteW *win = (NotaryNoteW*)d;
    win->checkThreadRunning=false;
    win->hide();
}
