
#include "selector.h"


static std::map<std::string, sel_t> selectors;

sel_t Sel(const std::string & selstr)
{
    std::map<std::string, sel_t>::iterator s = selectors.find(selstr);
    sel_t sel;
    if(s == selectors.end()) {
        sel = new std::string(selstr);
        selectors[selstr] = sel;
    }
    else {
        sel = s->second;
    }
    return sel;
}

