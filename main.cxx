//#include <winsock2.h> // under mingw
#include "icon.xpm"
#include "InternalThread.h"
#include "NewAccountW.h"
#include "TransferLiquiW.h"
#include "KeysHandling.h"
#include "LiquiditiesHandling.h"
#include "ConnectionHandling.h"
#include "RequestBuilder.h"
#include "MessageProcessor.h"
#include "Logger.h"
#include "ShowClaimsW.h"
#include "TransRequestsW.h"
#include "ShowTransInfoW.h"
#include "ShowKeyInfoW.h"
#include "ShowLiquiInfoW.h"
#include "FindRefThreadsW.h"
#include "ShowDecThreadW.h"
#include "ShowRefInfoW.h"
#include "DeleteLiquiW.h"
#include "ExchangeOffersW.h"
#include "ShowLineageInfoW.h"
#include "ShowNotaryInfoW.h"
#include "FindNotaryThreadsW.h"
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <list>

#define W 375
#define H 170

using namespace std;

Fl_Choice *chO;
Fl_Choice *chR;
Fl_Choice *chN;

list<Fl_Window*> subWins;

void onChoice(Fl_Widget *, void *);
void win_cb(Fl_Widget *, void *);
void exitRoutine();

KeysHandling *keys;
LiquiditiesHandling *liquidities;
RequestBuilder *requestBuilder;
ConnectionHandling *connection;
MessageProcessor *msgProcessor;
InternalThread *internalThread;
Logger *logger;

int main (int argc, char ** argv)
{
    // build logger or set to nullptr
    //logger = new Logger();
    logger = nullptr;

    // build window
    Fl::scheme("gleam");
    Fl::visible_focus(0);
    Fl_Double_Window *w = new Fl_Double_Window(W, H, "OCS Client Application");
    Fl_Pixmap* fl_p = new Fl_Pixmap((char *const *) ocs_icon);
    Fl_RGB_Image* fl_rgb = new Fl_RGB_Image(fl_p);
    w->icon(fl_rgb);
    w->default_icon(fl_rgb);
    w->iconlabel("OCS Client");
    w->default_xclass("OCS Client");

    // operator tasks choice
    Fl_Box *boxO = new Fl_Box(20,20,120,25);
    boxO->labelfont(FL_COURIER);
    boxO->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxO->label("Operator tasks:");
    chO = new Fl_Choice(150,20,200,25);
    chO->add("Select:");
    chO->add("   Show account state");
    chO->add("   Transfer liquidity");
    chO->add("   Request liquidity");
    chO->add("   Exchange liquidity");
    chO->add("   Delete liquidity");
    chO->add("   Show liquidity info");
    chO->add("   Show transaction info");
    chO->add("   Create new account");
    chO->add("   Show account info");
    chO->callback(onChoice, (void*)chO);
    chO->take_focus();
    chO->value(0);

    // referee tasks choice
    Fl_Box *boxR = new Fl_Box(20,60,120,25);
    boxR->labelfont(FL_COURIER);
    boxR->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxR->label("Referee tasks:");
    chR = new Fl_Choice(150,60,200,25);
    chR->add("Select:");
    chR->add("   Find decision threads");
    chR->add("   Show decision thread");
    chR->add("   Show currency info");
    chR->add("   Show referee info");
    chR->callback(onChoice, (void*)chR);
    chR->take_focus();
    chR->value(0);

    // notary tasks choice
    Fl_Box *boxN = new Fl_Box(20,100,120,25);
    boxN->labelfont(FL_COURIER);
    boxN->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    boxN->label("Notary tasks:");
    chN = new Fl_Choice(150,100,200,25);
    chN->add("Select:");
    chN->add("   Find decision threads");
    chN->add("   Show decision thread");
    chN->add("   Show lineage info");
    chN->add("   Show notary info");
    chN->callback(onChoice, (void*)chN);
    chN->take_focus();
    chN->value(0);

    // add status bar
    Fl_Box *statusbar = new Fl_Box(0,H-25,W,25);
    statusbar->box(FL_FLAT_BOX);
    statusbar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
    statusbar->color(46);
    statusbar->label("Not connected");

    w->callback(win_cb);
    w->end();
    Fl::lock();
    w->show(argc, argv);

    // NOTE: comment in for mingw:
    /*
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        if (logger!=nullptr) logger->error("WSAStartup failed");
    }
    */

    // build background objects
    keys = new KeysHandling();
    liquidities = new LiquiditiesHandling();
    requestBuilder = new RequestBuilder(keys, liquidities);
    requestBuilder->setLogger(logger);
    connection = new ConnectionHandling(requestBuilder, statusbar);
    connection->setLogger(logger);
    msgProcessor = new MessageProcessor();
    msgProcessor->setLogger(logger);
    // start connection thread
    connection->startConnector(msgProcessor);
    // start internal thread
    internalThread = new InternalThread(connection);
    internalThread->setLogger(logger);
    internalThread->start();

    return(Fl::run());
}

void onChoice(Fl_Widget *w, void *d)
{
    Fl_Choice *ch = (Fl_Choice*)w;
    if (ch == chO)
    {
        if (strcmp(ch->text(), "   Show transaction info") == 0)
        {
            ShowTransInfoW *showTransInfoW = new ShowTransInfoW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showTransInfoW);
            ((Fl_Window*)showTransInfoW)->show();
        }
        else if (strcmp(ch->text(), "   Create new account") == 0)
        {
            NewAccountW *newAccountW = new NewAccountW(keys);
            subWins.insert(subWins.end(), (Fl_Window*)newAccountW);
            ((Fl_Window*)newAccountW)->show();
        }
        else if (strcmp(ch->text(), "   Show account info") == 0)
        {
            ShowKeyInfoW *showKeyInfoW = new ShowKeyInfoW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showKeyInfoW);
            ((Fl_Window*)showKeyInfoW)->show();
        }
        else if (strcmp(ch->text(), "   Show liquidity info") == 0)
        {
            ShowLiquiInfoW *showLiquiInfoW = new ShowLiquiInfoW(&subWins, internalThread, false);
            subWins.insert(subWins.end(), (Fl_Window*)showLiquiInfoW);
            ((Fl_Window*)showLiquiInfoW)->show();
        }
        else
        {
            // some tests
            list<string> publicKeysNames;
            keys->loadPublicKeysNames(publicKeysNames);
            if (publicKeysNames.empty())
            {
                fl_alert("Error: no stored accounts.");
                chO->value(0);
                return;
            }
            list<string> liquisNames;
            liquidities->loadKnownLiquidities(liquisNames);
            if (liquisNames.empty())
            {
                fl_alert("Error: no stored currencies or obligations.");
                chO->value(0);
                return;
            }
        }
        // continue
        if (strcmp(ch->text(), "   Show account state") == 0)
        {
            ShowClaimsW *showClaimsW = new ShowClaimsW(internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showClaimsW);
            ((Fl_Window*)showClaimsW)->show();
        }
        else if (strcmp(ch->text(), "   Transfer liquidity") == 0)
        {
            TransferLiquiW *transferLiquiW = new TransferLiquiW(internalThread, logger);
            subWins.insert(subWins.end(), (Fl_Window*)transferLiquiW);
            ((Fl_Window*)transferLiquiW)->show();
        }
        else if (strcmp(ch->text(), "   Exchange liquidity") == 0)
        {
            ExchangeOffersW *exchangeOffersW = new ExchangeOffersW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)exchangeOffersW);
            ((Fl_Window*)exchangeOffersW)->show();
        }
        else if (strcmp(ch->text(), "   Delete liquidity") == 0)
        {
            DeleteLiquiW *deleteLiquiW = new DeleteLiquiW(internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)deleteLiquiW);
            ((Fl_Window*)deleteLiquiW)->show();
        }
        else if (strcmp(ch->text(), "   Request liquidity") == 0)
        {
            TransRequestsW *transRequestsW = new TransRequestsW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)transRequestsW);
            ((Fl_Window*)transRequestsW)->show();
        }
        chO->value(0);
    }
    else if (ch == chR)
    {
        if (strcmp(ch->text(), "   Show decision thread") == 0)
        {
            ShowDecThreadW *showDecThreadW = new ShowDecThreadW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showDecThreadW);
            ((Fl_Window*)showDecThreadW)->show();
        }
        else if (strcmp(ch->text(), "   Show currency info") == 0)
        {
            ShowLiquiInfoW *showLiquiInfoW = new ShowLiquiInfoW(&subWins, internalThread, true);
            subWins.insert(subWins.end(), (Fl_Window*)showLiquiInfoW);
            ((Fl_Window*)showLiquiInfoW)->show();
        }
        else
        {
            list<string> liquisNames;
            liquidities->loadKnownLiquidities(liquisNames);
            if (liquisNames.empty())
            {
                fl_alert("Error: no stored currencies or obligations.");
                chR->value(0);
                return;
            }
        }
        // continue
        if (strcmp(ch->text(), "   Find decision threads") == 0)
        {
            FindRefThreadsW *findRefThreadsW = new FindRefThreadsW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)findRefThreadsW);
            ((Fl_Window*)findRefThreadsW)->show();
        }
        else if (strcmp(ch->text(), "   Show referee info") == 0)
        {
            ShowRefInfoW *showRefInfoW = new ShowRefInfoW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showRefInfoW);
            ((Fl_Window*)showRefInfoW)->show();
        }
        chR->value(0);
    }
    else if (ch == chN)
    {
        if (strcmp(ch->text(), "   Find decision threads") == 0)
        {
            FindNotaryThreadsW *findNotaryThreadsW = new FindNotaryThreadsW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)findNotaryThreadsW);
            ((Fl_Window*)findNotaryThreadsW)->show();
        }
        else if (strcmp(ch->text(), "   Show decision thread") == 0)
        {
            ShowDecThreadW *showDecThreadW = new ShowDecThreadW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showDecThreadW);
            ((Fl_Window*)showDecThreadW)->show();
        }
        else if (strcmp(ch->text(), "   Show lineage info") == 0)
        {
            ShowLineageInfoW *showLineageInfoW = new ShowLineageInfoW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showLineageInfoW);
            ((Fl_Window*)showLineageInfoW)->show();
        }
        else if (strcmp(ch->text(), "   Show notary info") == 0)
        {
            ShowNotaryInfoW *showNotaryInfoW = new ShowNotaryInfoW(&subWins, internalThread);
            subWins.insert(subWins.end(), (Fl_Window*)showNotaryInfoW);
            ((Fl_Window*)showNotaryInfoW)->show();
        }
        chN->value(0);
    }
}

void win_cb(Fl_Widget *widget, void *)
{
    bool shownWins = false;
    list<Fl_Window*>::iterator it;
    for (it=subWins.begin(); it!=subWins.end() && !shownWins; ++it)
    {
        Fl_Window* subWin = *it;
        shownWins = shownWins || subWin->shown();
    }

    if (!shownWins) exitRoutine();

    int answer = fl_choice("Do you want to close the application and all subwindows?",
                           "No", "Yes", 0);
    if (answer == 1) exitRoutine();
}

void exitRoutine()
{
    internalThread->stopSafely();
    delete internalThread;
    connection->stopSafely();
    delete connection;
    delete msgProcessor;
    delete requestBuilder;
    delete keys;
    // NOTE: Comment in for mingw:
    /*
    if (WSACleanup() == SOCKET_ERROR)
    {
        if (logger!=nullptr) logger->error("WSACleanup failed");
    }
    else if (logger!=nullptr) logger->info("WSACleanup completed");
    */
    if (logger!=nullptr) delete logger;
    exit(0);
}
