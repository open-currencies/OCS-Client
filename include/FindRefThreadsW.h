#ifndef FINDREFTHREADSW_H
#define FINDREFTHREADSW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include <string>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include "Fl_Html_View.H"

class FindRefThreadsW : Fl_Window
{
public:
    FindRefThreadsW(list<Fl_Window*> *s, InternalThread *i);
    ~FindRefThreadsW();
protected:
private:
    static list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling* keys;
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

    Fl_Box *box4;
    Fl_Choice *ch4;

    Fl_Box *box5;
    Fl_Input *inp;

    Fl_Button *but1;

    Fl_Html_View *view;

    volatile unsigned long rqstNum;

    volatile pthread_t *waitForListThread;

    static void onRequest(Fl_Widget *w, void *d);
    static void* waitForListRoutine(void *w);
};

#endif // FINDREFTHREADSW_H
