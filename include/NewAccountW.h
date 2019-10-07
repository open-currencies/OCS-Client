#ifndef NEWACCOUNTW_H
#define NEWACCOUNTW_H

#include "KeysHandling.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <string>
#include "rsa.h"
#include "osrng.h"
#include "files.h"
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

class NewAccountW : Fl_Window
{
public:
    NewAccountW(KeysHandling *k);
    NewAccountW(KeysHandling *k, CompleteID &id);
    ~NewAccountW();
protected:
private:
    Fl_Window *w;
    Fl_Box *box;
    Fl_Input *inp;
    Fl_Button *but;
    KeysHandling *keys;

    Fl_Box *boxId;
    CompleteID keyID;

    static void onCreate(Fl_Widget *w, void *d);
    static void onAdd(Fl_Widget *w, void *d);
    static bool isGood(string &s);
};

#endif // NEWACCOUNTW_H
