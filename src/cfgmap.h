
#ifndef CFGMAP_H
#define CFGMAP_H

#include <string>
#include <map>

//******************************************************************************

// A simple text-based key-value store.
//   Read and write key-value parameters in newline-terminated, colon-separated format.
// Each line starts with the key, which consists of all characters up to the colon, and
// the value consists of all characters from the colon up to the first newline.
//   All printable characters are written directly, with one exception: \ characters are
// escaped as \005C. All unprintable characters are similarly escaped, so arbitrary binary
// data can be stored or even hand-written.

void ReadParams(const std::string & path, std::map<std::string, std::string> & params);
void WriteParams(const std::string & path, const std::map<std::string, std::string> & params);

//******************************************************************************
#endif // CFGMAP_H
