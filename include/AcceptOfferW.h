#ifndef ACCEPTOFFERW_H
#define ACCEPTOFFERW_H

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
#include <pthread.h>
#include <math.h>

using namespace std;

class AcceptOfferW : Fl_Window
{
public:
    AcceptOfferW(InternalThread *i, string lR, double aR, string lO, double aO, CIDsSet *targetT10EId, CompleteID oID);
    ~AcceptOfferW();
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

    Fl_Box *box2;
    Fl_Choice *ch2;

    Fl_Box *box3;
    Fl_Input *inpR;

    Fl_Box *box4;
    Fl_Input *inpO;

    Fl_Button *but1;
    Fl_Button *but2;

    string source;
    string liquiR;
    string liquiO;
    double amountR;
    double amountO;

    CIDsSet *targetId;
    CompleteID ownerId;

    static void onSend(Fl_Widget *w, void *d);
    static void onChoice(Fl_Widget *w, void *d);
    static void onClose(Fl_Widget *w, void *d);

    volatile bool checkThreadRunning;
    volatile bool dataStatusClear;
    pthread_t checkDataThread;
    static void* checkDataRoutine(void *w);
};

#endif // ACCEPTOFFERW_H
