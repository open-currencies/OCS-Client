#ifndef SHOWLINEAGEINFOW_H
#define SHOWLINEAGEINFOW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "hex.h"
#include "NMECpp.h"
#include "NotaryApplicationW.h"
#include <algorithm>
#include <climits>
#include <pthread.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include "Fl_Html_View.H"
#include <string>
#include <iostream>
#include <cstring>
#include <map>

class ShowLineageInfoW : Fl_Window
{
public:
    ShowLineageInfoW(list<Fl_Window*> *s, InternalThread *i);
    ~ShowLineageInfoW();
protected:
private:
    static list<Fl_Window*> *subWins;
    static InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    static KeysHandling* keys;
    MessageProcessor *msgProcessor;

    Fl_Button *but1;
    Fl_Button *but2;
    Fl_Html_View *b;

    static void onApplication(Fl_Widget *w, void *d);
    volatile unsigned long rqstNum;
    volatile pthread_t *waitForDataThread;
    static void onReload(Fl_Widget *w, void *d);
    static void* waitForDataRoutine(void *w);
};

#endif // SHOWLINEAGEINFOW_H
