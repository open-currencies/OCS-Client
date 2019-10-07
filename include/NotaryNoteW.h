#ifndef NOTARYNOTEW_H
#define NOTARYNOTEW_H

#include "InternalThread.h"
#include "RequestBuilder.h"
#include "ConnectionHandling.h"
#include "NMECpp.h"
#include "CreoleCheatSheatW.h"
#include "ShowDecThreadW.h"
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

class NotaryNoteW : Fl_Window
{
public:
    NotaryNoteW(list<Fl_Window*> *s, InternalThread *i, list<pair<Type12Entry*, CIDsSet*>>* t);
    ~NotaryNoteW();
protected:
private:
    static list<Fl_Window*> *subWins;
    InternalThread *internal;
    RequestBuilder *requestBuilder;
    ConnectionHandling *connection;
    KeysHandling* keys;
    MessageProcessor *msgProcessor;

    Fl_Group *group0;
    Fl_Box *box0;
    Fl_Choice *ch0;
    Fl_Box *dum0;

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

    list<pair<Type12Entry*, CIDsSet*>>* thread;
    CompleteID applicationId;
    CompleteID threadId;

    string referee;

    static void onCreole(Fl_Widget *w, void *d);
    static void onLoad(Fl_Widget *w, void *d);
    static void onSave(Fl_Widget *w, void *d);
    static void onProceed(Fl_Widget *w, void *d);
    static void onBack(Fl_Widget *w, void *d);
    static void onSend(Fl_Widget *w, void *d);
    static void onChoice(Fl_Widget *w, void *d);

    volatile bool checkThreadRunning;
    volatile bool dataStatusClear;
    pthread_t checkDataThread;
    static void* checkDataRoutine(void *w);
    static void onClose(Fl_Widget *w, void *d);
};

#endif // NOTARYNOTEW_H
