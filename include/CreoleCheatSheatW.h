#ifndef CREOLECHEATSHEATW_H
#define CREOLECHEATSHEATW_H
#include <FL/Fl_Window.H>
#include <string>
#include "Fl_Html_View.H"

class CreoleCheatSheatW : Fl_Window
{
public:
    CreoleCheatSheatW();
    ~CreoleCheatSheatW();
protected:
private:
    Fl_Html_View *view;
};

#endif // CREOLECHEATSHEATW_H
