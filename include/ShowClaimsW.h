#ifndef SHOWCLAIMSW_H
#define SHOWCLAIMSW_H

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
#include "Logger.h"

using namespace std;

class ShowClaimsW : Fl_Window
{
public:
    ShowClaimsW(InternalThread *i);
    ~ShowClaimsW();
    void setLogger(Logger *l);
protected:
private:
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling *keys;
    LiquiditiesHandling *liquis;
    MessageProcessor *msgProcessor;

    Fl_Group *group1;
    Fl_Box *box0;
    Fl_Choice *ch0;
    Fl_Box *dum1;

    Fl_Group *group2;
    Fl_Box *box1;
    Fl_Choice *ch1;
    Fl_Box *dum2;

    Fl_Group *group3;
    Fl_Box *box3;
    Fl_Input *inp;
    Fl_Box *dum3;

    Fl_Button *but1;
    Fl_Button *but2;

    Fl_Html_View *b;

    string account;
    string liqui;
    volatile unsigned long rqstNum;

    volatile pthread_t *waitForListThread;

    static void onChoice(Fl_Widget *w, void *d);
    static void onRequest(Fl_Widget *w, void *d);
    static void onNewClaim(Fl_Widget *w, void *d);
    static void* waitForListRoutine(void *w);

    Logger *log;
    void logInfo(const char *msg);
    void logError(const char *msg);
};

#endif // SHOWCLAIMSW_H
