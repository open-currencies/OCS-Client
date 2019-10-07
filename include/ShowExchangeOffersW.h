#ifndef SHOWEXCHANGEOFFERSW_H
#define SHOWEXCHANGEOFFERSW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include <algorithm>
#include <climits>
#include <pthread.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include "Fl_Html_View.H"
#include <string>
#include <iostream>
#include <cstring>

using namespace std;

class ShowExchangeOffersW : Fl_Window
{
public:
    ShowExchangeOffersW(InternalThread *i);
    ~ShowExchangeOffersW();
protected:
private:
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling *keys;
    LiquiditiesHandling *liquis;
    MessageProcessor *msgProcessor;

    Fl_Box *box0;
    Fl_Choice *ch0;

    Fl_Box *box1;
    Fl_Choice *ch1;

    Fl_Box *box2;
    Fl_Choice *ch2;

    Fl_Box *box3;
    Fl_Choice *ch3;

    Fl_Button *but1;

    Fl_Html_View *b;

    string account;
    string liquiO;
    string liquiR;
    volatile unsigned long rqstNum;

    pthread_t *waitForListThread;

    static void onChoice(Fl_Widget *w, void *d);
    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForListRoutine(void *w);
};

#endif // SHOWEXCHANGEOFFERSW_H
