#ifndef FINDNOTARYTHREADSW_H
#define FINDNOTARYTHREADSW_H

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

class FindNotaryThreadsW : Fl_Window
{
public:
    FindNotaryThreadsW(list<Fl_Window*> *s, InternalThread *i);
    ~FindNotaryThreadsW();
protected:
private:
    static list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling* keys;
    MessageProcessor *msgProcessor;

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

#endif // FINDNOTARYTHREADSW_H
