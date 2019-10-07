#include "ExchangeOffersW.h"

ExchangeOffersW::ExchangeOffersW(list<Fl_Window*> *s, InternalThread *i) : Fl_Window(230, 115, "Exchange offers"), subWins(s), internal(i)
{
    // show offers
    showOffers = new Fl_Button(25,25,180,25, "Show existing offers");
    showOffers->callback(ExchangeOffersW::onShow, (void*)this);

    // new offer button
    newOffer = new Fl_Button(25,65,180,25, "Create new offer");
    newOffer->callback(ExchangeOffersW::onNew, (void*)this);

    end();
}

ExchangeOffersW::~ExchangeOffersW()
{
    //dtor
}

void ExchangeOffersW::onShow(Fl_Widget *w, void *d)
{
    ExchangeOffersW *win = (ExchangeOffersW*)d;
    ShowExchangeOffersW *showExchangeOffersW = new ShowExchangeOffersW(win->internal);
    win->subWins->insert(win->subWins->end(), (Fl_Window*)showExchangeOffersW);
    ((Fl_Window*)showExchangeOffersW)->show();
    win->hide();
}

void ExchangeOffersW::onNew(Fl_Widget *w, void *d)
{
    ExchangeOffersW *win = (ExchangeOffersW*)d;
    NewExchangeOfferW *newExchangeOfferW = new NewExchangeOfferW(win->internal);
    win->subWins->insert(win->subWins->end(), (Fl_Window*)newExchangeOfferW);
    ((Fl_Window*)newExchangeOfferW)->show();
}
