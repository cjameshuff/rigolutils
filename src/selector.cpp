
#include "selector.h"


static std::map<std::string, sel_t> selectors;

sel_t Sel(const std::string & selstr)
{
    sel_t sel = new std::string(selstr);
    std::pair<std::map<std::string, sel_t>::iterator, bool> s = selectors.insert(std::make_pair(selstr, sel));
    if(s.second == false)
        delete sel;
    return s.first->second;
}

