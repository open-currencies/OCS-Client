#include "FeeAndStakeCalcW.h"

FeeAndStakeCalcW::FeeAndStakeCalcW(Type5Entry *t5e, CompleteID &cid, InternalThread *i) : Fl_Window(550, 215, "Fee and stake calculator"),
    type5entry(t5e), liquiId(cid), internal(i)
{
    connection = internal->getConnectionHandling();
    requestBuilder = connection->getRqstBuilder();
    liquis = requestBuilder->getLiquiditiesHandling();

    // print currency id (and possibly name)
    box0 = new Fl_Box(20,15,120,25);
    box0->labelfont(FL_COURIER);
    box0->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box0->label("Liquidity Id:");
    box1 = new Fl_Box(134,16,160,25);
    box1->labelfont(FL_COURIER);
    box1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box1->labelsize(10);
    box1->copy_label(liquiId.to27Char().c_str());
    string liqui = liquis->getNameById(liquiId);
    if (liqui.length()>0)
    {
        box2 = new Fl_Box(294,15,200,25);
        box2->labelfont(FL_COURIER);
        box2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        string dum(" (");
        dum.append(liqui);
        dum.append(")");
        box2->copy_label(dum.c_str());
    }

    // amount
    inpA = new Fl_Input(410,50,120,25);
    inpA->textfont(FL_COURIER);
    inpA->callback(onChangeA, (void*)this);
    boxA = new Fl_Box(140,50,250,25);
    boxA->labelfont(FL_COURIER);
    boxA->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxA->label("liquidity amount in request:");

    // minimal fee
    inpFmin = new Fl_Input(410,80,120,25);
    inpFmin->textcolor(fl_rgb_color(100,100,100));
    inpFmin->textfont(FL_COURIER);
    inpFmin->readonly(1);
    boxFmin = new Fl_Box(140,80,250,25);
    boxFmin->labelfont(FL_COURIER);
    boxFmin->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxFmin->label("minimal processing fee:");

    // fee
    inpF = new Fl_Input(410,110,120,25);
    inpF->textfont(FL_COURIER);
    inpF->callback(onChangeF, (void*)this);
    boxF = new Fl_Box(140,110,250,25);
    boxF->labelfont(FL_COURIER);
    boxF->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxF->label("processing fee:");

    // maximal fee
    inpFmax = new Fl_Input(410,140,120,25);
    inpFmax->textcolor(fl_rgb_color(100,100,100));
    inpFmax->textfont(FL_COURIER);
    inpFmax->readonly(1);
    boxFmax = new Fl_Box(140,140,250,25);
    boxFmax->labelfont(FL_COURIER);
    boxFmax->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxFmax->label("maximal processing fee:");

    // stake
    inpS = new Fl_Input(410,170,120,25);
    inpS->textcolor(fl_rgb_color(100,100,100));
    inpS->textfont(FL_COURIER);
    inpS->readonly(1);
    boxS = new Fl_Box(140,170,250,25);
    boxS->labelfont(FL_COURIER);
    boxS->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxS->label("referee stake:");

    end();
}

FeeAndStakeCalcW::~FeeAndStakeCalcW()
{
    if (type5entry!=nullptr) delete type5entry;
}

void FeeAndStakeCalcW::onChangeA(Fl_Widget *w, void *d)
{
    FeeAndStakeCalcW *win = (FeeAndStakeCalcW*)d;
    win->inpF->value("");
    win->inpS->value("");
    // get amount
    double amount=0;
    string str = string(win->inpA->value());
    if (str.length()<=0)
    {
        win->inpFmin->value("");
        win->inpFmax->value("");
        return;
    }
    try
    {
        amount = stod(str);
    }
    catch(invalid_argument& e)
    {
        win->inpFmin->value("");
        win->inpFmax->value("");
        return;
    }
    double minFee = win->type5entry->getMinimalFee(amount);
    double maxFee = win->type5entry->getMaximalFee(amount);
    Util u;
    win->inpFmin->value(u.toString(minFee,2).c_str());
    win->inpFmax->value(u.toString(maxFee,2).c_str());
}

void FeeAndStakeCalcW::onChangeF(Fl_Widget *w, void *d)
{
    FeeAndStakeCalcW *win = (FeeAndStakeCalcW*)d;
    // get amount
    double amount=0;
    string str = string(win->inpA->value());
    if (str.length()<=0)
    {
        win->inpFmin->value("");
        win->inpFmax->value("");
        win->inpF->value("");
        win->inpS->value("");
        return;
    }
    try
    {
        amount = stod(str);
    }
    catch(invalid_argument& e)
    {
        win->inpFmin->value("");
        win->inpFmax->value("");
        win->inpF->value("");
        win->inpS->value("");
        return;
    }
    // set min and max fee
    double minFee = win->type5entry->getMinimalFee(amount);
    double maxFee = win->type5entry->getMaximalFee(amount);
    Util u;
    win->inpFmin->value(u.toString(minFee,2).c_str());
    win->inpFmax->value(u.toString(maxFee,2).c_str());
    // get fee
    double fee=0;
    str = string(win->inpF->value());
    if (str.length()<=0)
    {
        win->inpS->value("");
        return;
    }
    try
    {
        fee = stod(str);
    }
    catch(invalid_argument& e)
    {
        win->inpS->value("");
        return;
    }

    if (!win->type5entry->isGoodFee(amount, fee))
    {
        win->inpS->value("");
        return;
    }
    double stake = win->type5entry->getRefereeStake(amount, fee);
    win->inpS->value(u.toString(stake,2).c_str());
}
