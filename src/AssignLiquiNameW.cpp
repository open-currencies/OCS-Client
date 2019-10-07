#include "AssignLiquiNameW.h"

AssignLiquiNameW::AssignLiquiNameW(LiquiditiesHandling *l, CompleteID &id) : Fl_Window(380, 85, "Add liquidity"), liquis(l), liquiID(id)
{
    box = new Fl_Box(10,10,100,25);
    box->labelfont(FL_COURIER);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->label("Local name for liquidity");

    boxId = new Fl_Box(200,11,100,25);
    boxId->labelfont(FL_COURIER);
    boxId->labelsize(10);
    boxId->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    string txt(" ");
    txt.append(liquiID.to27Char());
    txt.append(":");
    boxId->copy_label(txt.c_str());

    inp = new Fl_Input(80,40,200,25);
    but = new Fl_Button(300,40,60,25,"Save");
    but->callback(AssignLiquiNameW::onAdd, (void*)this);
    end();
}

AssignLiquiNameW::~AssignLiquiNameW()
{
    //dtor
}

void AssignLiquiNameW::onAdd(Fl_Widget *w, void *d)
{
    AssignLiquiNameW *win = (AssignLiquiNameW*)d;
    const char * seq = win->inp->value();
    string str(seq);
    if (str.length() < 1)
    {
        fl_alert("Error: %s", "name too short.");
        return;
    }
    else if (str.length() > 23)
    {
        fl_alert("Error: %s", "name too long.");
        return;
    }
    else if (!isGood(str))
    {
        fl_alert("Error: %s", "only latin letters, digits and \"_\" are allowed.");
        return;
    }
    if (!win->liquis->addLiqui(str, win->liquiID))
    {
        fl_alert("Error: %s", "could not add ID under this name.");
        return;
    }
    win->liquis->saveIDs();
    win->hide();
}

bool AssignLiquiNameW::isGood(string &s)
{
    bool out=true;
    for (unsigned int i=0; i<s.length() && out; i++)
    {
        out = out && (isalpha(s[i]) || isdigit(s[i]) || (s[i] == '_'));
    }
    return out;
}
