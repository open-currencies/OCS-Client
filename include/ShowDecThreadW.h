#ifndef SHOWDECTHREADW_H
#define SHOWDECTHREADW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "hex.h"
#include "NMECpp.h"
#include "RefereeNoteW.h"
#include "NotaryNoteW.h"
#include <algorithm>
#include <climits>
#include <pthread.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include "Fl_Html_View.H"
#include <string>
#include <iostream>
#include <cstring>
#include <map>

class ShowDecThreadW : Fl_Window
{
public:
    ShowDecThreadW(list<Fl_Window*> *s, InternalThread *i);
    ~ShowDecThreadW();
    static unsigned long long printOutThread(list<pair<Type12Entry*, CIDsSet*>>* thread, KeysHandling *keys, LiquiditiesHandling *liquis, string &htmlTxt);
protected:
private:
    list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling *keys;
    LiquiditiesHandling *liquis;
    MessageProcessor *msgProcessor;

    Fl_Box *box;
    Fl_Input *inp;
    Fl_Button *but;
    Fl_Html_View *b;

    list<pair<Type12Entry*, CIDsSet*>>* thread;

    volatile unsigned long rqstNum;
    volatile pthread_t *waitForDataThread;
    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForDataRoutine(void *w);
    static const char *linkRoutine(Fl_Widget *w, const char *uri);
    static mutex winByApplIdMutex;
    static map<string, ShowDecThreadW*> winByApplId;
};

#endif // SHOWDECTHREADW_H
