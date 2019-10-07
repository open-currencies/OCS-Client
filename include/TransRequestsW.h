#ifndef TRANSREQUESTSW_H
#define TRANSREQUESTSW_H

#include "InternalThread.h"
#include "NewTransRequestW.h"
#include "ShowTransRqstsW.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <list>

using namespace std;

class TransRequestsW : Fl_Window
{
public:
    TransRequestsW(list<Fl_Window*> *s, InternalThread *i);
    ~TransRequestsW();
protected:
private:
    Fl_Button *showRequests;
    Fl_Button *newRequest;

    list<Fl_Window*> *subWins;
    InternalThread *internal;

    static void onShow(Fl_Widget *w, void *d);
    static void onNew(Fl_Widget *w, void *d);
};

#endif // TRANSREQUESTSW_H
