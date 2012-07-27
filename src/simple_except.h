
#ifndef SIMPLE_EXCEPT_H
#define SIMPLE_EXCEPT_H

#include <string>
#include <stdexcept>
#include <stdarg.h>

class FormattedError: public std::exception {
  protected:
    std::string msg;
  public:
    FormattedError(const std::string & format, ...) {
        va_list ap;
        va_start(ap, format);
        char * fstr;
        vasprintf(&fstr, format.c_str(), ap);
        if(fstr != NULL) {
            msg = fstr;
            free(fstr);
        }
        va_end(ap);
    }
    virtual ~FormattedError() throw() {}
    virtual const char* what() const throw() {return msg.c_str();}
};

#endif // SIMPLE_EXCEPT_H
