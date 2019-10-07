#ifndef NEWTRANSREQUESTW_H
#define NEWTRANSREQUESTW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_ask.H>
#include <string>

using namespace std;

class NewTransRequestW : Fl_Window
{
public:
    NewTransRequestW(InternalThread *i);
    virtual ~NewTransRequestW();
protected:
private:
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling *keys;
    LiquiditiesHandling *liquis;

    Fl_Box *box0;
    Fl_Choice *ch0;

    Fl_Box *box1;
    Fl_Choice *ch1;

    Fl_Box *box3;
    Fl_Input *inp;

    Fl_Button *but1;

    string source;
    string liqui;

    static void onSend(Fl_Widget *w, void *d);
    static void onChoice(Fl_Widget *w, void *d);
};

#endif // NEWTRANSREQUESTW_H
