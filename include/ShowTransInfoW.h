#ifndef SHOWTRANSINFO_H
#define SHOWTRANSINFO_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "TransferLiquiW.h"
#include "AcceptOfferW.h"
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

class ShowTransInfoW : Fl_Window
{
public:
    ShowTransInfoW(list<Fl_Window*> *s, InternalThread *i);
    ~ShowTransInfoW();
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

    Type10Entry* requestEntry;

    volatile unsigned long rqstNum;
    pthread_t *waitForDataThread;
    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForDataRoutine(void *w);
    static const char *linkRoutine(Fl_Widget *w, const char *uri);
    static mutex winByTransIdMutex;
    static map<string, ShowTransInfoW*> winByTransId;
    static map<string, CompleteID> ownerByTransId;
    static map<string, CIDsSet*> idsByTransId;
};

#endif // SHOWTRANSINFO_H
