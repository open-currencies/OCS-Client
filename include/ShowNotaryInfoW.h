#ifndef SHOWNOTARYINFOW_H
#define SHOWNOTARYINFOW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "NewAccountW.h"
#include <algorithm>
#include <climits>
#include <pthread.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include "Fl_Html_View.H"
#include <string>
#include <iostream>
#include <cstring>
#include <map>
#include "hex.h"

class ShowNotaryInfoW : Fl_Window
{
public:
    ShowNotaryInfoW(list<Fl_Window*> *s, InternalThread *i);
    ~ShowNotaryInfoW();
protected:
private:
    static list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    static KeysHandling* keys;
    LiquiditiesHandling *liquis;
    MessageProcessor *msgProcessor;

    Fl_Box *box0;
    Fl_Choice *ch0;

    Fl_Box *box1;
    Fl_Choice *ch1;

    Fl_Box *box;
    Fl_Input *inp;

    Fl_Button *but;
    Fl_Html_View *b;

    string *publicKeyStr;

    volatile unsigned long rqstNum;
    volatile pthread_t *waitForDataThread;
    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForDataRoutine(void *w);
};

#endif // SHOWNOTARYINFOW_H
