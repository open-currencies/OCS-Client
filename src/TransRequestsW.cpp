#include "TransRequestsW.h"

TransRequestsW::TransRequestsW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(230, 115, "Transfer requests"), subWins(s), internal(i)
{
    // show requests
    showRequests = new Fl_Button(25,25,180,25, "Show existing requests");
    showRequests->callback(TransRequestsW::onShow, (void*)this);

    // new request button
    newRequest = new Fl_Button(25,65,180,25, "Create new request");
    newRequest->callback(TransRequestsW::onNew, (void*)this);

    end();
}

TransRequestsW::~TransRequestsW()
{
    //dtor
}

void TransRequestsW::onShow(Fl_Widget *w, void *d)
{
    TransRequestsW *win = (TransRequestsW*)d;
    ShowTransRqstsW *showTransRqstsW = new ShowTransRqstsW(win->internal);
    win->subWins->insert(win->subWins->end(), (Fl_Window*)showTransRqstsW);
    ((Fl_Window*)showTransRqstsW)->show();
    win->hide();
}

void TransRequestsW::onNew(Fl_Widget *w, void *d)
{
    TransRequestsW *win = (TransRequestsW*)d;
    NewTransRequestW *newTransRequestW = new NewTransRequestW(win->internal);
    win->subWins->insert(win->subWins->end(), (Fl_Window*)newTransRequestW);
    ((Fl_Window*)newTransRequestW)->show();
}
