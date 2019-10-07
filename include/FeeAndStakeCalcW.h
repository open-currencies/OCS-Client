#ifndef FEEANDSTAKECALCW_H
#define FEEANDSTAKECALCW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <string>

class FeeAndStakeCalcW : Fl_Window
{
public:
    FeeAndStakeCalcW(Type5Entry *t5e, CompleteID &cid, InternalThread *i);
    ~FeeAndStakeCalcW();
protected:
private:
    Type5Entry *type5entry;
    CompleteID liquiId;

    InternalThread *internal;
    ConnectionHandling *connection;
    RequestBuilder *requestBuilder;
    LiquiditiesHandling *liquis;

    Fl_Box *box0;
    Fl_Box *box1;
    Fl_Box *box2;

    Fl_Box *boxA;
    Fl_Input *inpA;

    Fl_Box *boxFmin;
    Fl_Input *inpFmin;

    Fl_Box *boxFmax;
    Fl_Input *inpFmax;

    Fl_Box *boxF;
    Fl_Input *inpF;

    Fl_Box *boxS;
    Fl_Input *inpS;

    static void onChangeA(Fl_Widget *w, void *d);
    static void onChangeF(Fl_Widget *w, void *d);
};

#endif // FEEANDSTAKECALCW_H
