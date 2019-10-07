#include "NotaryApplicationW.h"

list<Fl_Window*>* NotaryApplicationW::subWins;

NotaryApplicationW::NotaryApplicationW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(640, 450, "New notary application"), internal(i)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();

    // choose applicant
    group0 = new Fl_Group(20, 10, 400, 25);
    box0 = new Fl_Box(20,10,100,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box0->label("Applicant:");
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

    // create html area
    box6 = new Fl_Box(20,50,100,25);
    box6->labelfont(FL_COURIER);
    box6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box6->label("Application text:");
    box6->hide();
    view = new Fl_Html_View(20, 80, 600, 325, 5);
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

    // check account
    if (ch0->text()!=nullptr) applicant=string(ch0->text());
    CompleteID keyID = keys->getInitialID(applicant);
    internal->addIdOfInterest(keyID);
    // check notary status
    string rqst;
    string *publicKeyStr = keys->getPublicKeyStr(applicant);
    if (requestBuilder->createNotaryInfoRqst(publicKeyStr, rqst))
    {
        connection->sendRequest(rqst);
    }

    end();
    resizable(editor);
}

NotaryApplicationW::~NotaryApplicationW()
{
    //dtor
}

void NotaryApplicationW::onChoice(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
    string str(win->ch0->text());
    if (str.compare(win->applicant) != 0)
    {
        win->applicant=str;
        CompleteID keyID = win->keys->getInitialID(win->applicant);
        win->internal->addIdOfInterest(keyID);
        // check notary status
        string rqst;
        string *publicKeyStr = win->keys->getPublicKeyStr(win->applicant);
        if (win->requestBuilder->createNotaryInfoRqst(publicKeyStr, rqst))
        {
            win->connection->sendRequest(rqst);
        }
    }
}

void NotaryApplicationW::onCreole(Fl_Widget *w, void *d)
{
    CreoleCheatSheatW *creoleCheatSheatW = new CreoleCheatSheatW();
    subWins->insert(subWins->end(), (Fl_Window*)creoleCheatSheatW);
    ((Fl_Window*)creoleCheatSheatW)->show();
}

void NotaryApplicationW::onLoad(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
    char *newfile;
    newfile = fl_file_chooser("Load text from...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->loadfile(newfile);
}

void NotaryApplicationW::onSave(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
    char *newfile;
    newfile = fl_file_chooser("Save text as...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->savefile(newfile);
}

void NotaryApplicationW::onProceed(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
    // check notary status
    string rqstStatus;
    string *publicKey = win->keys->getPublicKeyStr(win->applicant);
    if (win->requestBuilder->createNotaryInfoRqst(publicKey, rqstStatus))
    {
        win->connection->sendRequest(rqstStatus);
    }
    // check if notary is in keys
    TNtrNr tNtrNr = win->keys->getNotary(publicKey);
    if (!tNtrNr.isZero())
    {
        fl_alert("Error: this applicant is reported as notary.");
        return;
    }
    // get notary info and check
    NotaryInfo* notaryInfo = win->requestBuilder->getNotaryInfo(win->applicant);
    if (notaryInfo == nullptr)
    {
        fl_alert("Error: waiting for data on applicant.");
        return;
    }
    else
    {
        if (notaryInfo->getPendingOrApprovedApplicationId().getNotary()>0)
        {
            fl_alert("Error: %s", "existing pending (or approved) application reported.");
            return;
        }
    }

    // parse text
    NME nme(win->textbuf->text());
    nme.setFormat(NMEOutputFormatHTML);
    NMEConstText output;
    if (nme.getOutput(&output) == kNMEErrOk)
    {
        win->view->value((char*) output);
    }
    else
    {
        fl_alert("Error while parsing Creole.");
        return;
    }

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

void NotaryApplicationW::onBack(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
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

void NotaryApplicationW::onSend(Fl_Widget *w, void *d)
{
    NotaryApplicationW *win = (NotaryApplicationW*)d;
    if (!win->connection->connectionEstablished())
    {
        fl_alert("Error: %s", "no connection established.");
        return;
    }
    if (win->ch0->text()==nullptr)
    {
        fl_alert("Error: %s", "insufficient data supplied.");
        return;
    }
    // get applicant
    string applicant(win->ch0->text());
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
    bool result = win->requestBuilder->createNotaryApplRequest(applicant, description, notaryNr, rqst);

    // final checks
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }
    int answer = fl_choice("Are you sure you want to submit a new notary application\nfor account %s\nand that all displayed data is correct?\nNote that only one pending application per account is allowed.",
                           "Close", "No", "Yes", applicant.c_str());
    if (answer != 2) return;

    // rebuild request
    rqst="";
    result = win->requestBuilder->createNotaryApplRequest(applicant, description, notaryNr, rqst);
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }

    // send request
    win->connection->sendRequest(rqst);
    fl_alert("Info: %s", "a new notary application was sent for notarization.");
    win->hide();
}
