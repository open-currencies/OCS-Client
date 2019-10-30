#ifndef SHOWLIQUIINFOW_H
#define SHOWLIQUIINFOW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "NewAccountW.h"
#include "OperationProposalW.h"
#include "RefereeApplicationW.h"
#include "FeeAndStakeCalcW.h"
#include "AssignLiquiNameW.h"
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
#include "NMECpp.h"

class ShowLiquiInfoW : Fl_Window
{
public:
    ShowLiquiInfoW(list<Fl_Window*> *s, InternalThread *i, bool f);
    ~ShowLiquiInfoW();
protected:
private:
    bool forRefs;
    static list<Fl_Window*> *subWins;
    static InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    static KeysHandling* keys;
    static LiquiditiesHandling *liquis;
    MessageProcessor *msgProcessor;

    Type5Or15Entry* type5Or15entryC;

    Fl_Box *box0;
    Fl_Choice *ch0;

    Fl_Box *box;
    Fl_Input *inp;
    Fl_Button *but;
    Fl_Html_View *b;

    volatile unsigned long rqstNum;
    volatile pthread_t *waitForDataThread;
    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForDataRoutine(void *w);
    static const char *linkRoutine(Fl_Widget *w, const char *uri);
    static mutex winByLiquiIdMutex;
    static map<string, ShowLiquiInfoW*> winByLiquilId;
    static bool printOutInfoOnLiqui(CompleteID &firstId, Type5Or15Entry* t5Or15e, ShowLiquiInfoW *win);
};

#endif // SHOWLIQUIINFOW_H
