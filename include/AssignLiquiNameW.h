#ifndef ASSIGNLIQUINAMEW_H
#define ASSIGNLIQUINAMEW_H
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <sys/stat.h>
#include <string>
#include "LiquiditiesHandling.h"

using namespace std;

class AssignLiquiNameW : Fl_Window
{
public:
    AssignLiquiNameW(LiquiditiesHandling *l, CompleteID &id);
    ~AssignLiquiNameW();
protected:
private:
    Fl_Window *w;
    Fl_Box *box;
    Fl_Input *inp;
    Fl_Button *but;
    LiquiditiesHandling *liquis;

    Fl_Box *boxId;
    CompleteID liquiID;

    static void onAdd(Fl_Widget *w, void *d);
    static bool isGood(string &s);
};

#endif // ASSIGNLIQUINAMEW_H
