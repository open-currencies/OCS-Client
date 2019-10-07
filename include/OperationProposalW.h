#ifndef OPERATIONPROPOSALW_H
#define OPERATIONPROPOSALW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "NMECpp.h"
#include "CreoleCheatSheatW.h"
#include <FL/Fl_Window.H>
#include <string>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_File_Chooser.H>
#include "Fl_Html_View.H"

class OperationProposalW : Fl_Window
{
public:
    OperationProposalW(list<Fl_Window*> *s, InternalThread *i, string l);
    ~OperationProposalW();
protected:
private:
    static list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling* keys;
    LiquiditiesHandling *liquis;
    string liqui;

    Fl_Group *group0;
    Fl_Box *box0;
    Fl_Choice *ch0;
    Fl_Box *dum0;

    Fl_Group *group1;
    Fl_Box *box1;
    Fl_Input *inp1;
    Fl_Box *box3;
    Fl_Input *inp3;
    Fl_Box *dum1;

    Fl_Group *group2;
    Fl_Box *box2;
    Fl_Input *inp2;
    Fl_Box *box4;
    Fl_Input *inp4;
    Fl_Box *dum2;

    Fl_Group *group5;
    Fl_Box *box5;
    Fl_Button *but1;
    Fl_Box *dum5;

    Fl_Text_Buffer *textbuf;
    Fl_Text_Editor *editor;
    Fl_Button *but2;
    Fl_Button *but3;
    Fl_Button *but4;

    Fl_Box *box6;
    Fl_Html_View *view;
    Fl_Button *but5;
    Fl_Button *but6;

    static void onCreole(Fl_Widget *w, void *d);
    static void onLoad(Fl_Widget *w, void *d);
    static void onSave(Fl_Widget *w, void *d);
    static void onProceed(Fl_Widget *w, void *d);
    static void onBack(Fl_Widget *w, void *d);
    static void onSend(Fl_Widget *w, void *d);
    static void onChoice(Fl_Widget *w, void *d);

    string applicant;
    double amount;
    double fee;
    double forfeit;
    unsigned long processingTime;
};

#endif // OPERATIONPROPOSALW_H
