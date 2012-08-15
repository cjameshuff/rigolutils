//******************************************************************************
// Selector pool
// An interned string pool for converting strings into efficiently-comparable
// and storable selectors, aka symbols. Selectors can also be NULL.
// Note: the resulting selectors are simply pointers to strings and are not
// portable between systems or program instances.
//******************************************************************************

#ifndef SELECTOR_H
#define SELECTOR_H

#include <string>
#include <map>

typedef const std::string * sel_t;

sel_t Sel(const std::string & selstr);

//******************************************************************************
#endif // SELECTOR_H