#ifndef EXCHANGEOFFERSW_H
#define EXCHANGEOFFERSW_H

#include "InternalThread.h"
#include "NewExchangeOfferW.h"
#include "ShowExchangeOffersW.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <list>

using namespace std;

class ExchangeOffersW : Fl_Window
{
public:
    ExchangeOffersW(list<Fl_Window*> *s, InternalThread *i);
    ~ExchangeOffersW();
protected:
private:
    Fl_Button *showOffers;
    Fl_Button *newOffer;

    list<Fl_Window*> *subWins;
    InternalThread *internal;

    static void onShow(Fl_Widget *w, void *d);
    static void onNew(Fl_Widget *w, void *d);
};

#endif // EXCHANGEOFFERSW_H
