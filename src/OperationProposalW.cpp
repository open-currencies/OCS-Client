#include "OperationProposalW.h"

list<Fl_Window*>* OperationProposalW::subWins;

OperationProposalW::OperationProposalW(list<Fl_Window*> *s, InternalThread *i, string l) : Fl_Window(640, 480, "New operation proposal"),
    internal(i), liqui(l)
{
    subWins=s;
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    keys = requestBuilder->getKeysHandling();
    liquis = requestBuilder->getLiquiditiesHandling();

    // set title
    string title("New operation proposal in ");
    title.append(liqui);
    copy_label(title.c_str());

    // choose applicant
    group0 = new Fl_Group(20, 10, 400, 25);
    box0 = new Fl_Box(20,10,100,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box0->label("Applicant:");
    ch0 = new Fl_Choice(170,10,240,25);
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

    // requested amount
    group1 = new Fl_Group(20, 40, 620, 25);
    box1 = new Fl_Box(20,40,100,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box1->label("Requested amount:");
    inp1 = new Fl_Input(170,40,130,25);
    inp1->textsize(10);
    inp1->textfont(FL_COURIER);
    // Forfeit
    box3 = new Fl_Box(310,40,100,25);
    box3->labelfont(FL_COURIER);
    box3->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box3->label("Forfeit (optional):");
    inp3 = new Fl_Input(510,40,110,25);
    inp3->textsize(10);
    inp3->textfont(FL_COURIER);
    dum1 = new Fl_Box(625,40,5,25);
    dum1->hide();
    group1->resizable(dum1);
    group1->end();

    // processing fee
    group2 = new Fl_Group(20, 70, 620, 25);
    box2 = new Fl_Box(20,70,100,25);
    box2->labelfont(FL_COURIER);
    box2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box2->label("Processing fee:");
    inp2 = new Fl_Input(170,70,130,25);
    inp2->textsize(10);
    inp2->textfont(FL_COURIER);
    // processing time
    box4 = new Fl_Box(310,70,100,25);
    box4->labelfont(FL_COURIER);
    box4->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box4->label("Processing time (days):");
    inp4 = new Fl_Input(510,70,110,25);
    inp4->textsize(10);
    inp4->textfont(FL_COURIER);
    dum2 = new Fl_Box(625,70,5,25);
    dum2->hide();
    group2->resizable(dum2);
    group2->end();

    // edit area
    group5 = new Fl_Group(20, 110, 180, 25);
    box5 = new Fl_Box(20,110,100,25);
    box5->labelfont(FL_COURIER_ITALIC);
    box5->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box5->label("Please use ");
    but1 = new Fl_Button(108,110,65,25,"Creole:");
    but1->labelfont(FL_COURIER_ITALIC);
    but1->labelcolor(fl_rgb_color(0,0,255));
    but1->box(FL_NO_BOX);
    but1->callback(onCreole, (void*)this);
    dum5 = new Fl_Box(175,110,5,25);
    dum5->hide();
    group5->resizable(dum5);
    group5->end();

    textbuf = new Fl_Text_Buffer;
    editor = new Fl_Text_Editor(20, 140, 600, 295);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);
    editor->wrap_mode(1, 3000);
    but2 = new Fl_Button(500,110,120,25,"Load from file");
    but2->callback(onLoad, (void*)this);
    but3 = new Fl_Button(20,445,110,25,"Save to file");
    but3->callback(onSave, (void*)this);
    but4 = new Fl_Button(540,445,80,25,"Proceed");
    but4->callback(onProceed, (void*)this);

    // create html area
    box6 = new Fl_Box(20,110,100,25);
    box6->labelfont(FL_COURIER);
    box6->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box6->label("Proposal description:");
    box6->hide();
    view = new Fl_Html_View(20, 140, 600, 295, 5);
    view->color(FL_WHITE);
    view->format_width(-1);
    view->value("<p></p>");
    view->hide();
    but5 = new Fl_Button(540,445,80,25,"Back");
    but5->callback(onBack, (void*)this);
    but5->hide();
    but6 = new Fl_Button(290,445,60,25,"SEND");
    but6->callback(onSend, (void*)this);
    but6->hide();

    // update liqui
    CompleteID liquiID = liquis->getID(liqui);
    internal->addIdOfInterest(liquiID);
    // check account state
    if (ch0->text()!=nullptr) applicant=string(ch0->text());
    CompleteID keyID = keys->getInitialID(applicant);
    internal->addIdOfInterest(keyID);
    internal->addAccountPairOfInterest(applicant, liqui);

    end();
    resizable(editor);
}

OperationProposalW::~OperationProposalW()
{
    //dtor
}

void OperationProposalW::onChoice(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
    string str(win->ch0->text());
    if (str.compare(win->applicant) != 0)
    {
        win->applicant=str;
        CompleteID keyID = win->keys->getInitialID(win->applicant);
        win->internal->addIdOfInterest(keyID);
        win->internal->addAccountPairOfInterest(win->applicant, win->liqui);
    }
}

void OperationProposalW::onCreole(Fl_Widget *w, void *d)
{
    CreoleCheatSheatW *creoleCheatSheatW = new CreoleCheatSheatW();
    subWins->insert(subWins->end(), (Fl_Window*)creoleCheatSheatW);
    ((Fl_Window*)creoleCheatSheatW)->show();
}

void OperationProposalW::onLoad(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
    char *newfile;
    newfile = fl_file_chooser("Load text from...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->loadfile(newfile);
}

void OperationProposalW::onSave(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
    char *newfile;
    newfile = fl_file_chooser("Save text as...", "*.txt", "", 0);
    if (newfile == NULL) return;
    win->textbuf->savefile(newfile);
}

void OperationProposalW::onProceed(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
    // load type 5 entry
    Type5Entry *type5entry = (Type5Entry*) win->liquis->getLiqui(win->liqui);
    if (type5entry==nullptr)
    {
        fl_alert("Error: %s", "unknown currency.");
        return;
    }
    Util u;
    // load amount
    win->amount=0;
    string str = string(win->inp1->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no amount supplied.");
        return;
    }
    try
    {
        win->amount = stod(str);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad amount supplied.");
        return;
    }
    if (win->amount<=0)
    {
        fl_alert("Error: %s", "amount must be positive.");
        return;
    }
    if (win->amount>type5entry->getLiquidityCreationLimit())
    {
        fl_alert("Error: %s %s.", "maximal amount is", u.toString(type5entry->getLiquidityCreationLimit(), 2).c_str());
        return;
    }
    // load fee
    win->fee=0;
    str = string(win->inp2->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no fee supplied.");
        return;
    }
    try
    {
        win->fee = stod(str);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad fee supplied.");
        return;
    }
    if (win->fee<0)
    {
        fl_alert("Error: %s", "fee cannot be negative.");
        return;
    }
    if (win->fee < type5entry->getMinimalFee(win->amount))
    {
        fl_alert("Error: %s %s.", "minimal fee for this amount is", u.toString(type5entry->getMinimalFee(win->amount), 2).c_str());
        return;
    }
    if (win->fee > type5entry->getMaximalFee(win->amount))
    {
        fl_alert("Error: %s %s.", "maximal fee for this amount is", u.toString(type5entry->getMinimalFee(win->amount), 2).c_str());
        return;
    }
    // load forfeit
    win->forfeit=0;
    str = string(win->inp3->value());
    if (str.length()>0)
    {
        try
        {
            win->forfeit = stod(str);
        }
        catch(invalid_argument& e)
        {
            fl_alert("Error: %s", "bad forfeit supplied.");
            return;
        }
    }
    if (win->forfeit < 0)
    {
        fl_alert("Error: %s", "forfeit cannot be negative.");
        return;
    }
    if (win->forfeit >= win->amount)
    {
        fl_alert("Error: %s", "forfeit cannot exceed amount.");
        return;
    }
    // load processing time
    win->processingTime=0;
    str = string(win->inp4->value());
    if (str.length()<=0)
    {
        fl_alert("Error: %s", "no processing time supplied.");
        return;
    }
    try
    {
        win->processingTime = static_cast<unsigned long> (stod(str) * 60 * 60 * 24);
    }
    catch(invalid_argument& e)
    {
        fl_alert("Error: %s", "bad processing time supplied.");
        return;
    }
    if (win->processingTime < type5entry->minOpPrProcessingTime())
    {
        double minProcessingTimeInDays = 1.0 / 60 / 60 / 24 * type5entry->minOpPrProcessingTime();
        fl_alert("Error: %s %s %s.", "processing time must be above", u.toString(minProcessingTimeInDays, 2).c_str(), "days.");
        return;
    }
    if (win->processingTime > type5entry->maxOpPrProcessingTime())
    {
        double maxProcessingTimeInDays = 1.0 / 60 / 60 / 24 * type5entry->maxOpPrProcessingTime();
        fl_alert("Error: %s %s %s.", "processing time must be below", u.toString(maxProcessingTimeInDays, 2).c_str(), "days.");
        return;
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
    win->inp1->readonly(1);
    win->inp2->readonly(1);
    win->inp3->readonly(1);
    win->inp4->readonly(1);

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

void OperationProposalW::onBack(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
    win->view->hide();
    win->but5->hide();
    win->but6->hide();
    win->box6->hide();

    win->ch0->activate();
    win->inp1->readonly(0);
    win->inp2->readonly(0);
    win->inp3->readonly(0);
    win->inp4->readonly(0);

    win->editor->show();
    win->box5->show();
    win->but1->show();
    win->but2->show();
    win->but3->show();
    win->but4->show();
}

void OperationProposalW::onSend(Fl_Widget *w, void *d)
{
    OperationProposalW *win = (OperationProposalW*)d;
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
    // get claim id
    Type14Entry* t14e = win->requestBuilder->getLatestT14Ecopy(applicant, win->liqui);
    if (t14e==nullptr)
    {
        fl_alert("Error: unknown account state. Please check this account.");
        return;
    }
    // check liquiditiy
    if (t14e->getTransferableAmount() < win->forfeit)
    {
        fl_alert("Error: %s", "insufficient liquidity to cover forfeit.");
        delete t14e;
        return;
    }
    // build description
    string description("<!doctype creole>");
    description.append(win->textbuf->text());
    Util u;
    if (u.descriptionTooLarge(description))
    {
        fl_alert("Error: %s %u %s", "description too large. Limit:", u.descriptionLimitInKB(), "KB");
        delete t14e;
        return;
    }
    // build request
    string rqst;
    unsigned long notaryNr = win->connection->getNotaryNr();
    bool result = win->requestBuilder->createOpPrRequest(applicant, win->liqui, win->amount, win->fee, win->forfeit, win->processingTime, description, notaryNr, rqst);
    delete t14e;

    // final checks
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }
    int answer = fl_choice("Are you sure you want to submit a new operation proposal for \n%s units of liquidity %s for the account %s\nand that all displayed data is correct?",
                           "Close", "No", "Yes", u.toString(win->amount, 2).c_str(), win->liqui.c_str(), applicant.c_str());
    if (answer != 2) return;

    // rebuild request
    rqst = "";
    result = win->requestBuilder->createOpPrRequest(applicant, win->liqui, win->amount, win->fee, win->forfeit, win->processingTime, description, notaryNr, rqst);
    if (!result)
    {
        fl_alert("Error: %s", "insufficient data to process request.");
        return;
    }

    // send request
    win->connection->sendRequest(rqst);
    fl_alert("Info: %s", "a new operation proposal was sent for notarization.");
    win->hide();
}
